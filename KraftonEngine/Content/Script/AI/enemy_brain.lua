-- =============================================================================
--  enemy_brain.lua — 적 AI 정책 레이어
-- =============================================================================
--  C++은 센서/블랙보드/이동/공격 실행만 제공하고, 행동 선택·공격 점수·반응·페이즈
--  정책은 이 Lua 계층에서 결정한다.
-- =============================================================================

local STYLE = {
    Passive    = { attack=0.0, approach=0.4, strafe=0.8, retreat=1.2, deflect=0.15, react=0.35 },
    Balanced   = { attack=1.0, approach=1.0, strafe=0.7, retreat=0.35, deflect=0.75, react=0.55 },
    Aggressive = { attack=1.45, approach=1.15, strafe=0.45, retreat=0.15, deflect=0.45, react=0.45 },
    Defensive  = { attack=0.75, approach=0.75, strafe=0.95, retreat=0.85, deflect=1.45, react=0.75 },
    Boss       = { attack=1.30, approach=1.0, strafe=0.55, retreat=0.20, deflect=1.15, react=0.85 },
}

local TACTIC = {
    Neutral     = "Neutral",
    Opener      = "Opener",
    Pressure    = "Pressure",
    Combo       = "Combo",
    GapCloser   = "GapCloser",
    Punish      = "Punish",
    Retreat     = "Retreat",
    PhaseChange = "PhaseChange",
}

local S = nil

local LOCOMOTION = {
    Locked = 0,
    InputAllowed = 1,
    Strafe = 2,
    Retreat = 3,
}

local function call(o, fn, ...)
    if not o then return nil end
    return Reflection.Call(o, fn, ...)
end

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function name_to_string(v)
    if v == nil then return "" end
    return tostring(v)
end

local function is_name_valid(v)
    local s = name_to_string(v)
    return s ~= "" and s ~= "None"
end

local function current_style()
    if S and S.isBoss then return STYLE.Boss end
    return STYLE.Balanced
end

local function range_curve(distance, minRange, maxRange)
    local span = math.max(1.0, maxRange - minRange)
    local t = clamp((distance - minRange) / span, 0.0, 1.0)
    local off = math.abs(t - 0.5) * 2.0
    return 1.0 - 0.6 * off
end

local function angle_curve(absAngle, maxAbsAngle)
    local limit = math.max(1.0, maxAbsAngle)
    local t = clamp(absAngle / limit, 0.0, 1.0)
    return 1.0 - 0.7 * t
end

local function norm_tag(v)
    local s = tostring(v or "Neutral")
    s = s:gsub("_", ""):gsub("%s+", "")
    return s:lower()
end

local function tactic_scale(tactic, isGapCloser, targetThreatening, targetRecovery, targetPosture, phase, style)
    local tag = norm_tag(tactic)
    local scale = 1.0
    if tag == norm_tag(TACTIC.Opener) then
        scale = scale * (call(obj, "Brain_GetStateTime") <= 1.5 and 1.35 or 0.75)
    elseif tag == norm_tag(TACTIC.Pressure) then
        scale = scale * (1.0 + 0.35 * style.attack)
    elseif tag == norm_tag(TACTIC.Combo) then
        scale = scale * (is_name_valid(call(obj, "Brain_GetLastAttackName")) and 1.55 or 0.35)
    elseif tag == norm_tag(TACTIC.GapCloser) or isGapCloser then
        scale = scale * (1.05 + 0.30 * style.approach)
    elseif tag == norm_tag(TACTIC.Punish) then
        scale = scale * (targetRecovery and 1.65 or 0.85)
    elseif tag == norm_tag(TACTIC.Retreat) then
        scale = scale * (0.7 + style.retreat)
    elseif tag == norm_tag(TACTIC.PhaseChange) then
        scale = scale * (S.isBoss and 1.35 or 0.55)
    end

    if targetThreatening and (tag == norm_tag(TACTIC.Punish) or tag == norm_tag(TACTIC.Retreat)) then
        scale = scale * 1.25
    end
    if targetPosture > 0.0 and targetPosture < 0.4 and (tag == norm_tag(TACTIC.Pressure) or tag == norm_tag(TACTIC.Combo)) then
        scale = scale * 1.35
    end
    if phase >= 2 and (tag == norm_tag(TACTIC.Pressure) or tag == norm_tag(TACTIC.GapCloser) or tag == norm_tag(TACTIC.PhaseChange) or isGapCloser) then
        scale = scale * (1.0 + 0.12 * (phase - 1))
    end
    return scale
end

local function combo_gate(index)
    if not call(obj, "Brain_AttackRequiresPreviousAttack", index) then
        return true
    end
    local required = call(obj, "Brain_GetAttackRequiredPreviousAttack", index)
    return is_name_valid(required) and name_to_string(required) == name_to_string(call(obj, "Brain_GetLastAttackName"))
end

local function select_attack()
    local count = call(obj, "Brain_GetAttackCount") or 0
    if count <= 0 then return false end

    local style = current_style()
    local distance = call(obj, "Brain_GetDistance") or 9999.0
    local absAngle = call(obj, "Brain_GetAbsAngle") or 180.0
    local phase = call(obj, "Brain_GetPhase") or 1
    local targetPosture = call(obj, "Brain_GetTargetPostureRatio") or 1.0
    local targetRecovery = call(obj, "Brain_TargetInRecovery") == true
    local targetThreatening = call(obj, "Brain_TargetThreatening") == true
    local perceptualScale = ((call(obj, "Brain_CanSeeTarget") == true and call(obj, "Brain_HasLineOfSight") == true) or call(obj, "Brain_IsInProximity") == true) and 1.0 or 0.0

    call(obj, "Brain_BeginDecisionTrace")

    local bestName = nil
    local bestScore = -1.0

    for i = 0, count - 1 do
        if call(obj, "Brain_CanUseAttackIndex", i) and combo_gate(i) then
            local name = call(obj, "Brain_GetAttackName", i)
            local weight = call(obj, "Brain_GetAttackBaseWeight", i) or 0.0
            local priority = call(obj, "Brain_GetAttackPriority", i) or 1.0
            local minRange = call(obj, "Brain_GetAttackMinRange", i) or 0.0
            local maxRange = call(obj, "Brain_GetAttackMaxRange", i) or 1.0
            local maxAngle = call(obj, "Brain_GetAttackMaxAbsAngle", i) or 70.0
            local repeatScale = call(obj, "Brain_GetAttackRepeatWeightScale", i) or 1.0
            local repeatCount = call(obj, "Brain_GetRecentAttackRepeat", name) or 0
            local tactic = call(obj, "Brain_GetAttackTacticTag", i) or "Neutral"
            local isGapCloser = call(obj, "Brain_IsAttackGapCloser", i) == true

            local score = math.max(0.0001, weight)
                * math.max(0.1, priority)
                * range_curve(distance, minRange, maxRange)
                * angle_curve(absAngle, maxAngle)
                * math.pow(clamp(repeatScale, 0.0, 1.0), repeatCount)
                * tactic_scale(tactic, isGapCloser, targetThreatening, targetRecovery, targetPosture, phase, style)
                * style.attack
                * perceptualScale

            score = score * (0.92 + math.random() * 0.16)
            call(obj, "Brain_AddDecisionCandidate", name, score)

            if score > bestScore then
                bestScore = score
                bestName = name
            end
        end
    end

    if bestName and bestScore > 0.05 and call(obj, "Brain_SetSelectedAttack", bestName) then
        call(obj, "Brain_CommitDecisionTrace", bestName)
        return true
    end

    call(obj, "Brain_CommitDecisionTrace", "None")
    return false
end

local function should_deflect(style)
    if call(obj, "Brain_TargetThreatening") ~= true then return false end
    local distance = call(obj, "Brain_GetDistance") or 9999.0
    local range = call(obj, "Brain_GetAttackRange") or 3.0
    local angle = call(obj, "Brain_GetAbsAngle") or 180.0
    if distance > range * 1.8 or angle > 80.0 then return false end
    if not ((call(obj, "Brain_CanSeeTarget") == true and call(obj, "Brain_HasLineOfSight") == true) or call(obj, "Brain_IsInProximity") == true) then
        return false
    end
    return math.random() < style.react * style.deflect
end

local function decide_movement(style)
    local distance = call(obj, "Brain_GetDistance") or 9999.0
    local range = call(obj, "Brain_GetAttackRange") or 3.0
    local hp = call(obj, "Brain_GetSelfHealthRatio") or 1.0

    if distance > range then
        call(obj, "Brain_Chase")
        return
    end

    local retreatScore = style.retreat * ((hp < 0.35) and 1.7 or 1.0) * ((distance < range * 0.55) and 1.35 or 0.7)
    local strafeScore = style.strafe * (0.85 + math.random() * 0.3)
    if retreatScore > strafeScore then
        call(obj, "Brain_Reposition")
    else
        call(obj, "Brain_Strafe")
    end
end

local function define_runtime_states()
    local sm = call(obj, "GetStateMachine")
    if not sm then return end
    call(sm, "ClearStateDefinitions")
    call(sm, "DefineState", "Idle", LOCOMOTION.InputAllowed, 0, true, false)
    call(sm, "DefineState", "Chase", LOCOMOTION.InputAllowed, 1, true, true)
    call(sm, "DefineState", "Strafe", LOCOMOTION.Strafe, 2, true, true)
    call(sm, "DefineState", "Reposition", LOCOMOTION.Retreat, 3, true, true)
    call(sm, "DefineState", "Attack", LOCOMOTION.Locked, 4, false, false)
    call(sm, "DefineState", "Deflect", LOCOMOTION.Locked, 5, true, true)
    call(sm, "DefineState", "Guard", LOCOMOTION.Locked, 5, true, true)
    call(sm, "DefineState", "Recover", LOCOMOTION.InputAllowed, 6, true, false)
    call(sm, "DefineState", "Hit", LOCOMOTION.Locked, 7, true, false)
    call(sm, "DefineState", "Staggered", LOCOMOTION.Locked, 8, true, false)
    call(sm, "DefineState", "Dead", LOCOMOTION.Locked, 9, false, false)
    call(sm, "DefineState", "Intro", LOCOMOTION.Locked, 10, true, true)
    call(sm, "DefineState", "PhaseChanged", LOCOMOTION.Locked, 11, true, true)
    call(sm, "DefineState", "EncounterCompleted", LOCOMOTION.InputAllowed, 12, true, false)
end

function BeginPlay()
    define_runtime_states()
    S = {
        isBoss = type(call(obj, "GetBossPhase")) == "number",
    }
    pcall(function() math.randomseed((tonumber(obj.UUID) or os.time() or 1) + 13) end)
end

function EndPlay()
    call(obj, "Brain_ReleaseAttackToken")
end

function Tick(dt)
    if not S then return end

    call(obj, "Brain_Sense")
    if call(obj, "Brain_IsBusy") == true then return end
    if call(obj, "Brain_ConsumeCombatStep") ~= true then return end

    if call(obj, "Brain_AcquireTarget") ~= true then
        call(obj, "Brain_Idle")
        return
    end

    call(obj, "Brain_FaceTarget")

    local style = current_style()
    if should_deflect(style) then
        call(obj, "Brain_OpenDeflect")
        return
    end

    local distance = call(obj, "Brain_GetDistance") or 9999.0
    local range = call(obj, "Brain_GetAttackRange") or 3.0
    if distance <= range * 2.5 and style.attack > 0.0 then
        if call(obj, "Brain_AcquireAttackToken") == true then
            if select_attack() and call(obj, "Brain_PlaySelectedAttack") == true then
                return
            end
            call(obj, "Brain_ReleaseAttackToken")
        end
    end

    decide_movement(style)
end
