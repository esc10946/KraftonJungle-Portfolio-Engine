-- =============================================================================
--  enemy_brain.lua  —  소울/세키로형 적 AI 두뇌 (정책 레이어)
-- =============================================================================
--  설계: "결정은 Lua, 실행은 C++".
--    - C++ (actuator/sensor) : 길찾기, 거리/각도, 공격 데이터·몽타주, 포이즈/경직,
--                              위협 신호(CombatState:IsAttacking), Strafe/Retreat 동사.
--    - Lua  (이 파일)        : 매 think tick 마다 동사별 효용을 점수내 최고점을 실행.
--                              공격 / 접근 / 게걸음 / 후퇴 / 회피 / 패링 을 한 점수판에서
--                              경쟁시켜 "절묘한 조화" 를 만든다. 핫리로드로 즉시 튜닝.
--
--  부착:
--    AEnemyCharacter 의 ULuaScriptComponent.ScriptFile = "AI/enemy_brain.lua"
--    (BeginPlay 에서 bUseBuiltInDecisionLogic 을 자동으로 끈다.)
--
--  성향: AEnemyCharacter.BehaviorStyle (Passive/Balanced/Aggressive/Defensive/Boss)
--        → 아래 STYLES 테이블이 공격·방어·이동 가중치를 모두 관통한다.
-- =============================================================================

-- ── 성향 데이터 (전부 데이터. 빌드 없이 핫리로드로 튜닝) ──────────────────────
-- 키 = EEnemyAIBehaviorStyle enum 정수 (Passive=0 .. Boss=4)
local STYLES = {
    [0] = { name="Passive",    attack=0.0, approach=0.4, strafe=0.7, retreat=1.0,
            dodge=1.1, parry=0.2, reactChance=0.45, reactDelay=0.14 },
    [1] = { name="Balanced",   attack=1.0, approach=1.0, strafe=0.6, retreat=0.3,
            dodge=1.0, parry=0.8, reactChance=0.50, reactDelay=0.16 },
    [2] = { name="Aggressive", attack=1.5, approach=1.2, strafe=0.4, retreat=0.1,
            dodge=0.6, parry=0.35, reactChance=0.40, reactDelay=0.18 },
    [3] = { name="Defensive",  attack=0.8, approach=0.7, strafe=0.9, retreat=0.7,
            dodge=1.2, parry=1.7, reactChance=0.75, reactDelay=0.12 },
    [4] = { name="Boss",       attack=1.3, approach=1.0, strafe=0.5, retreat=0.2,
            dodge=1.0, parry=1.1, reactChance=0.85, reactDelay=0.14 },
}

-- ── 튜닝 상수 ────────────────────────────────────────────────────────────────
local THINK_INTERVAL       = 0.12   -- 의사결정 주기(초). 매 프레임이 아니라 이 간격으로만 생각.
local IFRAME_DURATION      = 0.45   -- 회피 무적(i-frame) 지속. SetInvincible 로 구현.
local PARRY_STANCE_DURATION= 0.30   -- 패링 스탠스(슈퍼아머) 지속. SetSuperArmor 로 구현.
local DODGE_MOVE_SCALE     = 1.0    -- 회피 시 후퇴 이동 강도.
local STRAFE_SCALE         = 0.7
local RETREAT_SCALE        = 0.7
local APPROACH_ACCEPT      = 0.85   -- 접근 정지 반경 = AttackRange * 이 값.
local CLOSE_RATIO          = 0.6    -- AttackRange * 이 값보다 가까우면 "너무 가까움".
local STRAFE_FLIP_PERIOD   = 1.6    -- 게걸음 방향(좌/우) 전환 주기(초).

local S = nil  -- 런타임 상태 (BeginPlay 에서 초기화)

-- ── 얇은 헬퍼 ────────────────────────────────────────────────────────────────
local function call(o, fn, ...)
    if not o then return nil end
    return Reflection.Call(o, fn, ...)
end

-- 어떤 액터든(플레이어 클래스 무관) CombatStateComponent 를 안전하게 얻는다.
local function get_combat(actor)
    if not actor then return nil end
    local ok, c = pcall(function() return actor:GetCombatStateComponent() end)
    if ok and c then return c end
    return nil
end

local function clamp(v, lo, hi)
    if v < lo then return lo elseif v > hi then return hi else return v end
end

-- ── 수명주기 ─────────────────────────────────────────────────────────────────
function BeginPlay()
    S = {
        brain  = obj:GetEnemyAIBrainComponent(),
        combat = obj:GetCombatStateComponent(),
        health = obj:GetHealthComponent(),
        now    = 0.0,
        think  = 0.0,
        intent = "idle",          -- 매 프레임 적용되는 지속 이동 의도
        clockwise = true,
        nextFlip = STRAFE_FLIP_PERIOD,
        reactKind = nil,          -- 예약된 반응("Dodge"/"Parry")
        reactAt   = 0.0,
        invincibleUntil = 0.0,    -- 회피 i-frame 만료 시각
        superArmorUntil = 0.0,    -- 패링 스탠스 만료 시각
    }

    -- C++ 내장 결정 로직을 끄고 이 Lua 두뇌가 운전대를 잡는다.
    Reflection.SetProperty(obj, "bUseBuiltInDecisionLogic", false)

    -- AttackRange 캐싱.
    local r = Reflection.GetProperty(S.brain, "AttackRange")
    S.attackRange = (type(r) == "number" and r > 0) and r or 3.0

    -- 보스 여부 1회 판별 (GetBossPhase 가 있으면 보스). 보스면 Boss 성향 강제.
    local bp = Reflection.Call(obj, "GetBossPhase")
    S.isBoss = (type(bp) == "number")

    -- 성향 결정.
    local idx
    if S.isBoss then
        idx = 4
    else
        local prop = Reflection.GetProperty(obj, "BehaviorStyle")
        idx = (type(prop) == "number") and prop or 1
    end
    S.style = STYLES[idx] or STYLES[1]

    -- 적마다 RNG 시드 분리 — 안 하면 모든 적이 같은 난수열로 똑같이 움직인다.
    pcall(function() math.randomseed((tonumber(obj.UUID) or 0) + 1) end)
end

function EndPlay()
    -- 종료 시 임시 상태(무적/슈퍼아머) 원복.
    if S then
        if S.invincibleUntil > 0 then call(S.combat, "SetInvincible", false) end
        if S.superArmorUntil > 0 then call(S.combat, "SetSuperArmor", false) end
    end
end

-- ── 임시 상태(i-frame / 패링 스탠스) 만료 처리 ───────────────────────────────
local function update_temp_states()
    if S.invincibleUntil > 0 and S.now >= S.invincibleUntil then
        call(S.combat, "SetInvincible", false)
        S.invincibleUntil = 0
    end
    if S.superArmorUntil > 0 and S.now >= S.superArmorUntil then
        call(S.combat, "SetSuperArmor", false)
        S.superArmorUntil = 0
    end
end

-- ── 반응 실행 (예약된 회피/패링을 reactDelay 후 발동 = 공정성) ────────────────
local function execute_reaction()
    if S.reactKind == "Dodge" then
        call(S.combat, "SetInvincible", true)
        S.invincibleUntil = S.now + IFRAME_DURATION
        S.intent = "dodge"                       -- i-frame 동안 후퇴 이동
        call(S.brain, "SetState", "Dodge")
    elseif S.reactKind == "Parry" then
        call(S.combat, "SetSuperArmor", true)
        S.superArmorUntil = S.now + PARRY_STANCE_DURATION
        S.intent = "hold"
        call(S.brain, "SetState", "Parry")
    end
    S.reactKind = nil
end

-- ── 매 프레임 적용되는 지속 이동 (think tick 이 정한 intent 를 부드럽게 유지) ──
local function apply_movement()
    if S.intent == "strafe" then
        call(obj, "StrafeAroundTarget", STRAFE_SCALE, S.clockwise)
    elseif S.intent == "retreat" then
        call(obj, "RetreatFromTarget", RETREAT_SCALE)
    elseif S.intent == "dodge" then
        call(obj, "RetreatFromTarget", DODGE_MOVE_SCALE)
    end
    -- "approach" 는 길찾기(RequestMoveToTarget)가 think tick 에서 처리. "hold"/"idle" 은 정지.
end

-- ── 의사결정 (think 주기마다 1회) ────────────────────────────────────────────
local function think()
    local brain = S.brain
    if not brain then return end

    -- 타깃 확보.
    if not call(brain, "HasValidTarget") then
        call(brain, "AcquireDefaultTarget")
    end
    if not call(brain, "HasValidTarget") then
        call(brain, "SetState", "Idle")
        call(brain, "StopMove")
        S.intent = "idle"
        return
    end

    -- 반응(회피/패링)이 이미 예약/진행 중이면 새 결정을 내리지 않고 대기.
    if S.reactKind or S.invincibleUntil > 0 or S.superArmorUntil > 0 then
        return
    end

    -- 컨텍스트 수집.
    local dist  = call(brain, "GetDistanceToTarget") or 9999
    local hp    = call(S.health, "GetHealthRatio") or 1.0
    local St    = S.style
    local inRange = dist <= S.attackRange

    -- 위협(타깃이 지금 공격 중인가) — 회피/패링의 근거.
    local target = call(brain, "GetTarget")
    local tcombat = get_combat(target)
    local threat = tcombat and (call(tcombat, "GetAttackThreatRemaining") or 0) or 0
    local underThreat = threat > 0 and dist <= (S.attackRange * 1.6)

    -- ① 위협 대응: 회피 vs 패링 을 성향 가중치로 선택 + 확률·지연으로 공정하게.
    if underThreat and not S.reactKind then
        if math.random() < St.reactChance then
            local pick = (St.parry * math.random() > St.dodge * math.random()) and "Parry" or "Dodge"
            S.reactKind = pick
            S.reactAt   = S.now + St.reactDelay   -- 즉시가 아니라 사람처럼 한 박자 늦게
            call(brain, "StopMove")
            S.intent = "hold"
            return
        end
        -- 확률 실패 → 그냥 평소 행동으로 흘려보냄(가끔 못 막는 게 공정함).
    end

    -- ② 평소: 동사별 효용 점수 → 최고점 실행. (약간의 랜덤으로 패턴화 방지)
    local score = {
        Attack   = inRange and St.attack or 0,
        Approach = (not inRange) and St.approach or 0,
        Strafe   = inRange and St.strafe or (St.strafe * 0.3),
        Retreat  = St.retreat
                   * ((hp < 0.3) and 1.8 or 1.0)
                   * ((dist < S.attackRange * CLOSE_RATIO) and 1.3 or 0.6),
    }

    local bestKey, bestVal = "Strafe", -1
    for k, v in pairs(score) do
        local jittered = v * (0.85 + math.random() * 0.3)
        if jittered > bestVal then bestVal, bestKey = jittered, k end
    end

    if bestKey == "Attack" then
        -- 어떤 공격인지는 C++ 가중치 시스템(SelectAttackForStyle)에 위임.
        local phase = S.isBoss and (call(obj, "GetBossPhase") or 1) or 1
        local res = call(obj, "SelectAndCommitAttack", phase)
        if type(res) == "table" and res.ReturnValue and res.OutAttack then
            call(obj, "PlayAttackMontage", res.OutAttack)   -- C++ Tick 이 스윙을 실행/종료
            S.intent = "hold"
        else
            S.intent = "strafe"   -- 쿨다운 등으로 공격 불가 → 간격 유지
        end
    elseif bestKey == "Approach" then
        call(brain, "SetState", "Chase")
        call(brain, "RequestMoveToTarget", S.attackRange * APPROACH_ACCEPT, true)
        S.intent = "approach"
    elseif bestKey == "Retreat" then
        call(brain, "SetState", "Reposition")
        call(brain, "StopMove")
        S.intent = "retreat"
    else -- Strafe
        call(brain, "SetState", "Strafe")
        call(brain, "StopMove")
        S.intent = "strafe"
    end
end

-- ── 메인 틱 ──────────────────────────────────────────────────────────────────
function Tick(dt)
    if not S then return end
    S.now = S.now + dt

    update_temp_states()

    -- 죽음/경직 중에는 두뇌 정지(C++ 가 처리).
    if call(obj, "IsDead") then
        S.intent = "idle"
        return
    end
    if call(S.combat, "IsStaggered") then
        call(S.brain, "StopMove")
        S.intent = "idle"
        return
    end

    -- 항상 타깃을 바라본다(가볍다). HasValidTarget 은 C++ 가 내부 체크.
    -- 두 번째 인자(OverrideYawRate)는 명시적으로 -1 → TurnSpeed 사용.
    call(obj, "FaceTarget", dt, -1.0)

    -- 공격 스윙 진행 중이면 C++ 가 실행. Lua 는 대기.
    if call(obj, "HasCurrentAttack") then
        return
    end

    -- 예약된 반응(회피/패링)을 지연 시간 경과 시 발동.
    if S.reactKind and S.now >= S.reactAt then
        execute_reaction()
    end

    -- 게걸음 방향 주기적 전환.
    S.nextFlip = S.nextFlip - dt
    if S.nextFlip <= 0 then
        S.clockwise = not S.clockwise
        S.nextFlip = STRAFE_FLIP_PERIOD
    end

    -- 의사결정은 think 주기마다.
    S.think = S.think - dt
    if S.think <= 0 then
        S.think = THINK_INTERVAL
        think()
    end

    -- think 이 정한 지속 이동을 매 프레임 적용(부드러운 이동).
    apply_movement()
end
