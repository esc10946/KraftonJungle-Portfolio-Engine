local HUD_DOCUMENT = "Content/Game/UI/HUD.rml"

local PLAYER_HP_MASK_ID = "player-hp-mask"
local BOSS_POSTURE_LEFT_MASK_ID = "boss-posture-left-mask"
local BOSS_POSTURE_RIGHT_MASK_ID = "boss-posture-right-mask"
local PLAYER_POSTURE_LEFT_MASK_ID = "player-posture-left-mask"
local PLAYER_POSTURE_RIGHT_MASK_ID = "player-posture-right-mask"

local PLAYER_TOKEN_ID = "player-icon-right"

local widget = nil
local lastHpRatio = nil
local lastBossPostureRatio = nil
local lastPlayerPostureRatio = nil
local lastTokenVisible = nil

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

function SetBossHUD(postureRatio)
    SetBossPostureRatio(postureRatio)
end

local function expose_hud_api()
    _G.HUD = _G.HUD or {}

    _G.HUD.SetPlayerHpRatio = SetPlayerHpRatio
    _G.HUD.SetPlayerPostureRatio = SetPlayerPostureRatio
    _G.HUD.SetBossPostureRatio = SetBossPostureRatio
    _G.HUD.SetPlayerTokenVisible = SetPlayerTokenVisible
    _G.HUD.SetPlayerHUD = SetPlayerHUD
    _G.HUD.SetBossHUD = SetBossHUD
end

local function clear_hud_api()
    if _G.HUD == nil then
        return
    end

    if _G.HUD.SetBossPostureRatio == SetBossPostureRatio then
        _G.HUD.SetPlayerHpRatio = nil
        _G.HUD.SetPlayerPostureRatio = nil
        _G.HUD.SetBossPostureRatio = nil
        _G.HUD.SetPlayerTokenVisible = nil
        _G.HUD.SetPlayerHUD = nil
        _G.HUD.SetBossHUD = nil
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

    SetPlayerHpRatio(0.9)
    SetBossPostureRatio(0.9)
    SetPlayerPostureRatio(0.1)
    SetPlayerTokenVisible(true)
end

function EndPlay()
    clear_hud_api()

    if widget ~= nil then
        widget:RemoveFromParent()
        widget = nil
    end

    lastHpRatio = nil
    lastBossPostureRatio = nil
    lastPlayerPostureRatio = nil
    lastTokenVisible = nil
end

function Tick(dt)
end
