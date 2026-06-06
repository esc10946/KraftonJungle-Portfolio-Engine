local HUD_DOCUMENT = "Content/Game/UI/HUD.rml"

local PLAYER_HP_MASK_ID = "player-hp-mask"
local BOSS_HP_MASK_ID = "boss-hp-mask"
local BOSS_POSTURE_LEFT_MASK_ID = "boss-posture-left-mask"
local BOSS_POSTURE_RIGHT_MASK_ID = "boss-posture-right-mask"
local BOSS_PANEL_ID = "boss-panel"
local BOSS_POSTURE_PANEL_ID = "boss-posture-panel"
local PLAYER_POSTURE_LEFT_MASK_ID = "player-posture-left-mask"
local PLAYER_POSTURE_RIGHT_MASK_ID = "player-posture-right-mask"

local PLAYER_TOKEN_ID = "player-icon-right"

local ENEMY_BAR_MAX = 16
local ENEMY_BAR_WIDTH = 120.0
local ENEMY_BAR_HEIGHT = 19.0
local ENEMY_BAR_ID_PREFIX = "enemy-bar-"
local ENEMY_HP_MASK_ID_PREFIX = "enemy-hp-mask-"
local ENEMY_POSTURE_MASK_ID_PREFIX = "enemy-posture-mask-"
local ENEMY_BAR_ACTOR_TAG = "Enemy"
local ENEMY_BAR_HEAD_PADDING_Z = 0.35
local ENEMY_BAR_FALLBACK_HEAD_OFFSET_Z = 3.5
local ENEMY_BAR_MAX_DISTANCE = 40.0
local ENEMY_BAR_SCREEN_EDGE_PADDING = 6.0
local ENEMY_BAR_UPDATE_INTERVAL = 1.0 / 60.0
local ENEMY_BAR_CHANGE_REVEAL_SECONDS = 3.0
local ENEMY_BAR_RATIO_CHANGE_EPSILON = 0.0001
local widget = nil
local playerHealth = nil
local playerHealthBindingId = nil
local playerPawnChangedBindingId = nil
local lastHpRatio = nil
local lastBossHpRatio = nil
local lastBossPostureRatio = nil
local lastBossHudVisible = nil
local lastPlayerPostureRatio = nil
local lastTokenVisible = nil
local enemyBarStates = {}
local enemySlotById = {}
local slotEnemyIdByIndex = {}
local enemyVisibilityById = {}
local lockOnEnemyId = nil
local enemyBarUpdateElapsed = 0.0
local hudTimeSeconds = 0.0

local function clamp01(value)
    value = tonumber(value) or 0.0

    if value < 0.0 then
        return 0.0
    end

    if value > 1.0 then
        return 1.0
    end

    return value
end

local function set_element_visible(element_id, visible)
    if widget == nil then
        return
    end

    if visible then
        widget:SetProperty(element_id, "display", "block")
    else
        widget:SetProperty(element_id, "display", "none")
    end
end

local function set_width_ratio(element_id, ratio)
    local width = string.format("%.1f%%", clamp01(ratio) * 100.0)
    widget:SetProperty(element_id, "width", width)
end

local function format_px(value)
    return string.format("%.1fpx", tonumber(value) or 0.0)
end

local function same_number(lhs, rhs)
    return lhs ~= nil and math.abs(lhs - rhs) <= 0.0001
end

local function normalize_enemy_bar_index(index)
    index = tonumber(index)

    if index == nil then
        return nil
    end

    index = math.floor(index)

    if index < 0 or index >= ENEMY_BAR_MAX then
        return nil
    end

    return index
end

local function get_enemy_bar_state(index)
    local state = enemyBarStates[index]

    if state == nil then
        state = {}
        enemyBarStates[index] = state
    end

    return state
end

local function safe_call(target, function_name, ...)
    if target == nil then
        return nil
    end

    local fn = target[function_name]

    if fn == nil then
        return nil
    end

    local ok, result = pcall(fn, target, ...)

    if not ok then
        return nil
    end

    return result
end

local is_valid_object

local function unbind_player_health()
    if playerHealthBindingId ~= nil and is_valid_object(playerHealth) then
        safe_call(playerHealth, "UnbindOnHealthChanged", playerHealthBindingId)
    end

    playerHealth = nil
    playerHealthBindingId = nil
end

local function bind_player_health_from_pawn(pawn)
    unbind_player_health()

    if not is_valid_object(pawn) then
        return false
    end

    local health = safe_call(pawn, "GetHealthComponent")

    if not is_valid_object(health) then
        return false
    end

    playerHealth = health
    playerHealthBindingId = safe_call(
        health,
        "BindOnHealthChanged",
        function(_component, _previousHealth, currentHealth, maxHealth)
            maxHealth = tonumber(maxHealth) or 0.0

            if maxHealth <= 0.0 then
                SetPlayerHpRatio(0.0)
                return
            end

            SetPlayerHpRatio((tonumber(currentHealth) or 0.0) / maxHealth)
        end
    )

    if playerHealthBindingId == nil or tonumber(playerHealthBindingId) == 0 then
        playerHealth = nil
        playerHealthBindingId = nil
        return false
    end

    SetPlayerHpRatio(safe_call(health, "GetHealthRatio") or 0.0)
    return true
end

local function bind_player_pawn_events()
    if World == nil or World.BindOnPlayerPawnChanged == nil then
        return false
    end

    playerPawnChangedBindingId = World.BindOnPlayerPawnChanged(
        function(_controller, _oldPawn, newPawn)
            bind_player_health_from_pawn(newPawn)
        end
    )

    if playerPawnChangedBindingId == nil or tonumber(playerPawnChangedBindingId) == 0 then
        playerPawnChangedBindingId = nil
        return false
    end

    local controller = World.GetFirstPlayerController and World.GetFirstPlayerController() or nil
    bind_player_health_from_pawn(safe_call(controller, "GetPossessedPawn"))
    return true
end

local function unbind_player_pawn_events()
    if playerPawnChangedBindingId ~= nil
        and World ~= nil
        and World.UnbindOnPlayerPawnChanged ~= nil then
        World.UnbindOnPlayerPawnChanged(playerPawnChangedBindingId)
    end

    playerPawnChangedBindingId = nil
end

is_valid_object = function(target)
    if target == nil then
        return false
    end

    local valid = safe_call(target, "IsValid")

    if valid == nil then
        return true
    end

    return valid == true
end

local function get_actor_id(actor)
    local id = tonumber(actor.UUID)

    if id ~= nil then
        return id
    end

    return tonumber(safe_call(actor, "GetUUID"))
end

local function get_actor_location(actor)
    local ok, location = pcall(function()
        return actor.Location
    end)

    if ok then
        return location
    end

    return nil
end

local function get_enemy_anchor(enemy)
    local location = get_actor_location(enemy)

    if location == nil then
        return nil
    end

    local offsetZ = ENEMY_BAR_FALLBACK_HEAD_OFFSET_Z
    local capsule = safe_call(enemy, "GetCapsuleComponent")

    if is_valid_object(capsule) then
        local halfHeight = tonumber(safe_call(capsule, "GetScaledCapsuleHalfHeight"))

        if halfHeight ~= nil and halfHeight > 0.0 then
            offsetZ = halfHeight + ENEMY_BAR_HEAD_PADDING_Z
        end
    end

    return Vec3(location.X, location.Y, location.Z + offsetZ)
end

local function is_boss_enemy(enemy)
    return is_valid_object(safe_call(enemy, "GetEncounterComponent"))
end

local function get_enemy_visibility_state(enemyId)
    local state = enemyVisibilityById[enemyId]

    if state == nil then
        state = {}
        enemyVisibilityById[enemyId] = state
    end

    return state
end

local function resolve_enemy_visibility_id(enemyOrId)
    local id = tonumber(enemyOrId)

    if id ~= nil then
        return id
    end

    if is_valid_object(enemyOrId) then
        return get_actor_id(enemyOrId)
    end

    return nil
end

local function is_enemy_in_combat_visibility_state(enemy)
    local brain = safe_call(enemy, "GetAIBrainComponent")

    if is_valid_object(brain) and safe_call(brain, "HasValidTarget") == true then
        return true
    end

    local combat = safe_call(enemy, "GetCombatStateComponent")

    if is_valid_object(combat) and safe_call(combat, "IsStaggered") == true then
        return true
    end

    if is_valid_object(combat) and safe_call(combat, "IsAttacking") == true then
        return true
    end

    return false
end

local function update_enemy_visibility_ratios(enemyId, hpRatio, postureRatio)
    local state = get_enemy_visibility_state(enemyId)
    hpRatio = clamp01(hpRatio)
    postureRatio = clamp01(postureRatio)

    if state.hasRatioSample == true then
        local hpChanged = math.abs((state.lastHpRatio or hpRatio) - hpRatio) > ENEMY_BAR_RATIO_CHANGE_EPSILON
        local postureChanged = math.abs((state.lastPostureRatio or postureRatio) - postureRatio) > ENEMY_BAR_RATIO_CHANGE_EPSILON

        if hpChanged or postureChanged then
            state.revealUntil = math.max(state.revealUntil or 0.0, hudTimeSeconds + ENEMY_BAR_CHANGE_REVEAL_SECONDS)
        end
    end

    state.lastHpRatio = hpRatio
    state.lastPostureRatio = postureRatio
    state.hasRatioSample = true
end

local function is_enemy_visibility_requested(enemy, enemyId)
    local state = enemyVisibilityById[enemyId]

    if state ~= nil then
        if state.manualVisible == true then
            return true
        end

        if state.manualUntil ~= nil and state.manualUntil > hudTimeSeconds then
            return true
        end

        if state.revealUntil ~= nil and state.revealUntil > hudTimeSeconds then
            return true
        end
    end

    return is_enemy_in_combat_visibility_state(enemy)
end

local function is_enemy_bar_too_far(screen)
    if ENEMY_BAR_MAX_DISTANCE <= 0.0 then
        return false
    end

    local distance = tonumber(screen.Distance)
    return distance ~= nil and distance > ENEMY_BAR_MAX_DISTANCE
end

local function is_enemy_bar_inside_safe_area(screen)
    local width = tonumber(screen.ViewportWidth)
    local height = tonumber(screen.ViewportHeight)

    if (width == nil or height == nil or width <= 1.0 or height <= 1.0)
        and Engine ~= nil and Engine.GetViewportSize ~= nil then
        local viewport = Engine.GetViewportSize()
        width = viewport and tonumber(viewport.Width) or width
        height = viewport and tonumber(viewport.Height) or height
    end

    if width == nil or height == nil or width <= 1.0 or height <= 1.0 then
        return true
    end

    local x = tonumber(screen.X) or 0.0
    local y = tonumber(screen.Y) or 0.0
    local minX = ENEMY_BAR_WIDTH * 0.5 + ENEMY_BAR_SCREEN_EDGE_PADDING
    local maxX = width - minX
    local minY = ENEMY_BAR_HEIGHT * 0.5 + ENEMY_BAR_SCREEN_EDGE_PADDING
    local maxY = height - minY

    return x >= minX and x <= maxX and y >= minY and y <= maxY
end

local function release_enemy_slot(slot)
    local enemyId = slotEnemyIdByIndex[slot]

    if enemyId ~= nil then
        enemySlotById[enemyId] = nil
    end

    slotEnemyIdByIndex[slot] = nil
    SetEnemyHealthBarVisible(slot, false)
end

local function clear_enemy_slot_mapping()
    enemySlotById = {}
    slotEnemyIdByIndex = {}
end

local function clear_enemy_visibility_state()
    enemyVisibilityById = {}
    lockOnEnemyId = nil
end

local function find_free_enemy_slot()
    for i = 0, ENEMY_BAR_MAX - 1 do
        if slotEnemyIdByIndex[i] == nil then
            return i
        end
    end

    return nil
end

local function acquire_enemy_slot(enemyId)
    local existing = enemySlotById[enemyId]

    if existing ~= nil then
        return existing
    end

    local slot = find_free_enemy_slot()

    if slot == nil then
        return nil
    end

    enemySlotById[enemyId] = slot
    slotEnemyIdByIndex[slot] = enemyId
    return slot
end

local function collect_enemy_bar_candidates()
    local candidates = {}

    if World == nil or World.FindActorsByTag == nil or Engine == nil or Engine.ProjectWorldToScreen == nil then
        return candidates
    end

    local enemies = World.FindActorsByTag(ENEMY_BAR_ACTOR_TAG)

    if enemies == nil then
        return candidates
    end

    local viewport = Engine.GetViewportSize and Engine.GetViewportSize() or nil
    local centerX = viewport and (tonumber(viewport.Width) or 0.0) * 0.5 or 0.0
    local centerY = viewport and (tonumber(viewport.Height) or 0.0) * 0.5 or 0.0

    for _, enemy in ipairs(enemies) do
        if is_valid_object(enemy) and not is_boss_enemy(enemy) then
            local enemyId = get_actor_id(enemy)
            local health = safe_call(enemy, "GetHealthComponent")

            if enemyId ~= nil and is_valid_object(health) and safe_call(health, "IsDead") ~= true then
                local combat = safe_call(enemy, "GetCombatStateComponent")
                local hpRatio = safe_call(health, "GetHealthRatio") or 1.0
                local postureRatio = 1.0

                if is_valid_object(combat) then
                    postureRatio = safe_call(combat, "GetPoiseRatio") or 1.0
                end

                update_enemy_visibility_ratios(enemyId, hpRatio, postureRatio)

                local anchor = get_enemy_anchor(enemy)
                local screen = anchor and Engine.ProjectWorldToScreen(anchor) or nil

                if screen ~= nil
                    and screen.Visible == true
                    and is_enemy_visibility_requested(enemy, enemyId)
                    and not is_enemy_bar_too_far(screen)
                    and is_enemy_bar_inside_safe_area(screen) then
                    local dx = (tonumber(screen.X) or 0.0) - centerX
                    local dy = (tonumber(screen.Y) or 0.0) - centerY

                    candidates[#candidates + 1] = {
                        id = enemyId,
                        x = tonumber(screen.X) or 0.0,
                        y = tonumber(screen.Y) or 0.0,
                        hp = hpRatio,
                        posture = postureRatio,
                        score = dx * dx + dy * dy,
                        hasSlot = enemySlotById[enemyId] ~= nil,
                    }
                end
            end
        end
    end

    table.sort(candidates, function(lhs, rhs)
        if lhs.hasSlot ~= rhs.hasSlot then
            return lhs.hasSlot
        end

        return lhs.score < rhs.score
    end)

    return candidates
end

local function update_enemy_health_bars()
    if widget == nil then
        return
    end

    local candidates = collect_enemy_bar_candidates()
    local selectedIds = {}
    local selectedCount = math.min(#candidates, ENEMY_BAR_MAX)

    for i = 1, selectedCount do
        selectedIds[candidates[i].id] = true
    end

    local releaseSlots = {}

    for enemyId, slot in pairs(enemySlotById) do
        if selectedIds[enemyId] ~= true then
            releaseSlots[#releaseSlots + 1] = slot
        end
    end

    for _, slot in ipairs(releaseSlots) do
        release_enemy_slot(slot)
    end

    for i = 1, selectedCount do
        local candidate = candidates[i]
        local slot = acquire_enemy_slot(candidate.id)

        if slot ~= nil then
            SetEnemyHealthBar(slot, candidate.x, candidate.y, candidate.hp, candidate.posture, true)
        end
    end
end

function SetPlayerHpRatio(ratio)
    if widget == nil then
        return
    end

    ratio = clamp01(ratio)

    if lastHpRatio ~= nil and math.abs(lastHpRatio - ratio) <= 0.0001 then
        return
    end

    set_width_ratio(PLAYER_HP_MASK_ID, ratio)

    lastHpRatio = ratio
end

function SetBossHpRatio(ratio)
    if widget == nil then
        return
    end

    ratio = clamp01(ratio)

    if lastBossHpRatio ~= nil and math.abs(lastBossHpRatio - ratio) <= 0.0001 then
        return
    end

    set_width_ratio(BOSS_HP_MASK_ID, ratio)

    lastBossHpRatio = ratio
end

function SetPlayerPostureRatio(ratio)
    if widget == nil then
        return
    end

    ratio = clamp01(ratio)

    if lastPlayerPostureRatio ~= nil and math.abs(lastPlayerPostureRatio - ratio) <= 0.0001 then
        return
    end

    set_width_ratio(PLAYER_POSTURE_LEFT_MASK_ID, ratio)
    set_width_ratio(PLAYER_POSTURE_RIGHT_MASK_ID, ratio)

    lastPlayerPostureRatio = ratio
end

function SetBossPostureRatio(ratio)
    if widget == nil then
        return
    end

    ratio = clamp01(ratio)

    if lastBossPostureRatio ~= nil and math.abs(lastBossPostureRatio - ratio) <= 0.0001 then
        return
    end

    set_width_ratio(BOSS_POSTURE_LEFT_MASK_ID, ratio)
    set_width_ratio(BOSS_POSTURE_RIGHT_MASK_ID, ratio)

    lastBossPostureRatio = ratio
end

function SetBossHUDVisible(visible)
    visible = visible == true

    if lastBossHudVisible ~= nil and lastBossHudVisible == visible then
        return
    end

    set_element_visible(BOSS_PANEL_ID, visible)
    set_element_visible(BOSS_POSTURE_PANEL_ID, visible)

    lastBossHudVisible = visible
end

function SetBossVisibility(visible)
    SetBossHUDVisible(visible)
end

function SetPlayerTokenVisible(visible)
    visible = visible == true

    if lastTokenVisible ~= nil and lastTokenVisible == visible then
        return
    end

    set_element_visible(PLAYER_TOKEN_ID, visible)

    lastTokenVisible = visible
end

function SetPlayerHUD(hpRatio, tokenVisible, postureRatio)
    SetPlayerHpRatio(hpRatio)
    SetPlayerTokenVisible(tokenVisible)

    if postureRatio ~= nil then
        SetPlayerPostureRatio(postureRatio)
    end
end

function SetBossHUD(hpRatio, postureRatio)
    if postureRatio == nil then
        SetBossPostureRatio(hpRatio)
        return
    end

    SetBossHpRatio(hpRatio)
    SetBossPostureRatio(postureRatio)
end

function SetEnemyHealthBarVisible(index, visible)
    if widget == nil then
        return
    end

    index = normalize_enemy_bar_index(index)

    if index == nil then
        return
    end

    visible = visible == true

    local state = get_enemy_bar_state(index)

    if state.visible ~= nil and state.visible == visible then
        return
    end

    set_element_visible(ENEMY_BAR_ID_PREFIX .. index, visible)
    state.visible = visible
end

function SetEnemyHealthBar(index, x, y, hpRatio, postureRatio, visible)
    if widget == nil then
        return
    end

    index = normalize_enemy_bar_index(index)

    if index == nil then
        return
    end

    if visible == nil and type(postureRatio) == "boolean" then
        visible = postureRatio
        postureRatio = hpRatio
    end

    visible = visible == true

    if not visible then
        SetEnemyHealthBarVisible(index, false)
        return
    end

    x = tonumber(x) or 0.0
    y = tonumber(y) or 0.0
    hpRatio = clamp01(hpRatio)
    postureRatio = clamp01(postureRatio)

    local left = x - ENEMY_BAR_WIDTH * 0.5
    local top = y - ENEMY_BAR_HEIGHT * 0.5
    local state = get_enemy_bar_state(index)
    local barId = ENEMY_BAR_ID_PREFIX .. index

    if state.visible ~= true then
        set_element_visible(barId, true)
        state.visible = true
    end

    if not same_number(state.left, left) then
        widget:SetProperty(barId, "left", format_px(left))
        state.left = left
    end

    if not same_number(state.top, top) then
        widget:SetProperty(barId, "top", format_px(top))
        state.top = top
    end

    if not same_number(state.hpRatio, hpRatio) then
        set_width_ratio(ENEMY_HP_MASK_ID_PREFIX .. index, hpRatio)
        state.hpRatio = hpRatio
    end

    if not same_number(state.postureRatio, postureRatio) then
        set_width_ratio(ENEMY_POSTURE_MASK_ID_PREFIX .. index, postureRatio)
        state.postureRatio = postureRatio
    end
end

function HideAllEnemyHealthBars()
    if widget ~= nil then
        for i = 0, ENEMY_BAR_MAX - 1 do
            SetEnemyHealthBarVisible(i, false)
        end
    end

    clear_enemy_slot_mapping()
    clear_enemy_visibility_state()
end

function SetEnemyHealthBarActorVisible(enemyOrId, visible, durationSeconds)
    local enemyId = resolve_enemy_visibility_id(enemyOrId)

    if enemyId == nil then
        return false
    end

    local state = get_enemy_visibility_state(enemyId)
    visible = visible == true

    if visible then
        durationSeconds = tonumber(durationSeconds)

        if durationSeconds ~= nil and durationSeconds > 0.0 then
            state.manualUntil = hudTimeSeconds + durationSeconds
        else
            state.manualVisible = true
            state.manualUntil = nil
        end

        return true
    end

    state.manualVisible = false
    state.manualUntil = nil
    state.revealUntil = nil

    local slot = enemySlotById[enemyId]

    if slot ~= nil then
        release_enemy_slot(slot)
    end

    return true
end

function SetEnemyHealthBarVisibility(enemyOrId, visible, durationSeconds)
    return SetEnemyHealthBarActorVisible(enemyOrId, visible, durationSeconds)
end

function SetEnemyHealthBarLockOnTarget(enemy)
    local enemyId = resolve_enemy_visibility_id(enemy)

    if lockOnEnemyId ~= nil and lockOnEnemyId ~= enemyId then
        SetEnemyHealthBarActorVisible(lockOnEnemyId, false)
    end

    lockOnEnemyId = enemyId

    if enemyId == nil then
        return false
    end

    return SetEnemyHealthBarActorVisible(enemyId, true)
end

function ClearEnemyHealthBarLockOnTarget()
    if lockOnEnemyId ~= nil then
        SetEnemyHealthBarActorVisible(lockOnEnemyId, false)
        lockOnEnemyId = nil
    end
end

local function expose_hud_api()
    _G.HUD = _G.HUD or {}

    _G.HUD.SetPlayerHpRatio = SetPlayerHpRatio
    _G.HUD.SetBossHpRatio = SetBossHpRatio
    _G.HUD.SetPlayerPostureRatio = SetPlayerPostureRatio
    _G.HUD.SetBossPostureRatio = SetBossPostureRatio
    _G.HUD.SetBossHUDVisible = SetBossHUDVisible
    _G.HUD.SetBossVisibility = SetBossVisibility
    _G.HUD.SetPlayerTokenVisible = SetPlayerTokenVisible
    _G.HUD.SetPlayerHUD = SetPlayerHUD
    _G.HUD.SetBossHUD = SetBossHUD
    _G.HUD.SetEnemyHealthBar = SetEnemyHealthBar
    _G.HUD.SetEnemyHealthBarVisible = SetEnemyHealthBarVisible
    _G.HUD.SetEnemyHealthBarActorVisible = SetEnemyHealthBarActorVisible
    _G.HUD.SetEnemyHealthBarVisibility = SetEnemyHealthBarVisibility
    _G.HUD.SetEnemyHealthBarLockOnTarget = SetEnemyHealthBarLockOnTarget
    _G.HUD.ClearEnemyHealthBarLockOnTarget = ClearEnemyHealthBarLockOnTarget
    _G.HUD.HideAllEnemyHealthBars = HideAllEnemyHealthBars
end

local function clear_hud_api()
    if _G.HUD == nil then
        return
    end

    if _G.HUD.SetBossPostureRatio == SetBossPostureRatio then
        _G.HUD.SetPlayerHpRatio = nil
        _G.HUD.SetBossHpRatio = nil
        _G.HUD.SetPlayerPostureRatio = nil
        _G.HUD.SetBossPostureRatio = nil
        _G.HUD.SetBossHUDVisible = nil
        _G.HUD.SetBossVisibility = nil
        _G.HUD.SetPlayerTokenVisible = nil
        _G.HUD.SetPlayerHUD = nil
        _G.HUD.SetBossHUD = nil
        _G.HUD.SetEnemyHealthBar = nil
        _G.HUD.SetEnemyHealthBarVisible = nil
        _G.HUD.SetEnemyHealthBarActorVisible = nil
        _G.HUD.SetEnemyHealthBarVisibility = nil
        _G.HUD.SetEnemyHealthBarLockOnTarget = nil
        _G.HUD.ClearEnemyHealthBarLockOnTarget = nil
        _G.HUD.HideAllEnemyHealthBars = nil
    end
end

function BeginPlay()
    widget = UI.CreateWidget(HUD_DOCUMENT)

    if widget == nil then
        print("[HUD] failed to create " .. HUD_DOCUMENT)
        return
    end

    widget:SetWantsMouse(false)
    widget:AddToViewportZ(0)
    expose_hud_api()
    HideAllEnemyHealthBars()
    SetBossHUDVisible(false)
    enemyBarUpdateElapsed = ENEMY_BAR_UPDATE_INTERVAL

    SetPlayerHpRatio(1.0)
    bind_player_pawn_events()
    SetBossHpRatio(1.0)
    SetBossPostureRatio(0.9)
    SetPlayerPostureRatio(1.4)
    SetPlayerTokenVisible(true)
end

function EndPlay()
    unbind_player_health()
    unbind_player_pawn_events()
    clear_hud_api()

    if widget ~= nil then
        widget:RemoveFromParent()
        widget = nil
    end

    lastHpRatio = nil
    lastBossHpRatio = nil
    lastBossPostureRatio = nil
    lastBossHudVisible = nil
    lastPlayerPostureRatio = nil
    lastTokenVisible = nil
    enemyBarStates = {}
    enemyBarUpdateElapsed = 0.0
    hudTimeSeconds = 0.0
    clear_enemy_slot_mapping()
    clear_enemy_visibility_state()
end

function Tick(dt)
    dt = tonumber(dt) or 0.0
    hudTimeSeconds = hudTimeSeconds + dt
    enemyBarUpdateElapsed = enemyBarUpdateElapsed + dt

    if enemyBarUpdateElapsed < ENEMY_BAR_UPDATE_INTERVAL then
        return
    end

    enemyBarUpdateElapsed = enemyBarUpdateElapsed - ENEMY_BAR_UPDATE_INTERVAL

    if enemyBarUpdateElapsed > ENEMY_BAR_UPDATE_INTERVAL then
        enemyBarUpdateElapsed = ENEMY_BAR_UPDATE_INTERVAL
    end

    update_enemy_health_bars()
end
