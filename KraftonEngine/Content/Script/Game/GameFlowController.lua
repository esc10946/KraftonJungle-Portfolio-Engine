-- ============================================================
-- GameFlowController.lua
-- Attach to a single controller actor placed in the GamePlay scene.
-- Owns macro game-flow input/UI wiring:
--   * ESC toggles pause through the GameMode (Game.TogglePause).
--   * On entering the Dead phase, fades the camera out and reddens the
--     death icon; REVIVE / GIVE IN resolve the phase.
-- Phase-driven UI is synced by polling Game.GetPhase() each Tick.
-- ============================================================

local PAUSE_ZORDER    = 20      -- above the gameplay HUD
local DEATH_ZORDER    = 20      -- pause and death are mutually exclusive phases
local REVIVE_ZORDER   = 20      -- revive flourish (brief, after death tears down)
local DEATH_FADE      = 2.5     -- seconds: camera fade-out + icon reddening
local GIVE_IN_FADE    = 0.4     -- seconds: GIVE IN rushes the fade to full black
local REVIVE_FLOURISH = 0.6     -- seconds: revive icon pop -> fade-out + dilate
local REVIVE_GROWTH   = 0.4     -- revive icon scales 1.0 -> 1.4 over the flourish

local pause  = nil              -- pause overlay, created hidden, shown while paused
local death  = nil              -- death overlay, created hidden, shown while Dead
local revive = nil              -- revive flourish overlay, shown briefly on revive

local deathActive   = false     -- death sequence currently running
local reviveActive  = false     -- revive flourish currently playing
local reviveElapsed = 0         -- seconds since the revive flourish started
local prevPhase     = nil       -- last polled phase, for edge detection

-- DEBUG ONLY: number keys fire macro flow events so we can exercise the state
-- machine without real combat. Set to false (or delete this block + the Tick
-- polling below) before shipping. Keys are edge-triggered (fire once per press).
local DEBUG_KEYS = true
local DEBUG_BINDINGS = {
    { key = "1", fn = function() Debug.PlayerDeath()  end },   -- die
    { key = "2", fn = function() Debug.PlayerRevive() end },   -- revive
    { key = "3", fn = function() Debug.BossSlain()    end },   -- boss defeated
    { key = "4", fn = function() Debug.Victory()      end },   -- win
    { key = "5", fn = function() Debug.Cutscene()     end },   -- enter cutscene
    { key = "6", fn = function() Debug.Leaderboard()  end },   -- leaderboard
    { key = "0", fn = function() Debug.PrintPhase()   end },   -- log current phase
}

-- Sync the overlay to the GameMode's pause state. TogglePause may be a no-op
-- (e.g. while dead), so we read the actual state rather than assuming it flipped.
local function SyncPauseUI()
    if pause == nil then return end
    if Engine.IsPaused() then
        pause:SetWantsMouse(true)   -- pause menu needs the cursor
        pause:AddToViewportZ(PAUSE_ZORDER)
    else
        pause:RemoveFromParent()
    end
end

local function ShowDeath()
    if death == nil then return end
    -- Reset to the dying state (the overlay persists across deaths).
    death:SetProperty("death-icon-red", "opacity", "0")
    death:SetText("give_in_btn", "GIVE IN")
    death:SetProperty("give_in_btn", "color", "#e8e8e8")
    death:SetProperty("title_btn", "display", "none")
    death:SetWantsMouse(true)
    death:AddToViewportZ(DEATH_ZORDER)
    CameraManager.FadeOut(DEATH_FADE)   -- post-process fade to black, held
    deathActive = true
end

-- True death: keep the overlay up (red icon held on black), grey out the spent
-- GIVE IN button, and reveal RETURN TO TITLE underneath.
local function ShowDefeated()
    if death == nil then return end
    death:SetProperty("death-icon-red", "opacity", "1")
    death:SetText("give_in_btn", "YOU DIED")          -- spent button -> status label
    death:SetProperty("give_in_btn", "color", "#666666")
    death:SetProperty("title_btn", "display", "block")
    deathActive = false   -- stop the fade/redden loop; the screen now persists
end

local function HideDeath()
    if death ~= nil then death:RemoveFromParent() end
    deathActive = false
end

-- Revive flourish: a self-contained burst (its own clock), independent of the
-- camera fade-in. Pops in at full alpha, then fades out while dilating. No
-- mouse capture — the player is back in control.
local function ShowRevive()
    if revive == nil then return end
    revive:SetProperty("revive-icon", "opacity", "1")
    revive:SetProperty("revive-icon", "transform", "scale(1.0)")
    revive:AddToViewportZ(REVIVE_ZORDER)
    reviveActive  = true
    reviveElapsed = 0
end

local function HideRevive()
    if revive ~= nil then revive:RemoveFromParent() end
    reviveActive = false
end

function BeginPlay()
    pause = UI.CreateWidget("Content/Game/UI/Pause.rml")
    if pause == nil then
        print("[GameFlowController] failed to create Pause.rml widget")
    else
        pause:bind_click("resume_btn", function()
            Game.TogglePause()
            SyncPauseUI()
        end)
        pause:bind_click("title_btn", function()
            Game.QuitToTitle()
        end)
    end

    death = UI.CreateWidget("Content/Game/UI/Death.rml")
    if death == nil then
        print("[GameFlowController] failed to create Death.rml widget")
    else
        -- Buttons only change phase; the Tick phase-transition drives the UI
        -- so debug keys behave the same. GIVE IN is inert once defeated.
        -- GIVE IN doesn't defeat directly; it rushes the camera fade to full
        -- black. The icon tracks the fade, and auto-defeat fires once it's black.
        death:bind_click("give_in_btn", function()
            if Game.GetPhase() == "Dead" then
                Game.CameraFade(Game.GetCameraFade(), 1, GIVE_IN_FADE)
            end
        end)
        death:bind_click("title_btn", function() Game.QuitToTitle() end)
    end

    revive = UI.CreateWidget("Content/Game/UI/Revive.rml")
    if revive == nil then
        print("[GameFlowController] failed to create Revive.rml widget")
    end

    -- ESC is read un-gated by the engine, so this fires while paused too.
    Engine.SetOnEscape(function()
        Game.TogglePause()
        SyncPauseUI()
    end)
end

function EndPlay()
    Engine.SetOnEscape(nil)
    if pause ~= nil then
        pause:RemoveFromParent()
        pause = nil
    end
    if death ~= nil then
        death:RemoveFromParent()
        death = nil
    end
    if revive ~= nil then
        revive:RemoveFromParent()
        revive = nil
    end
end

function Tick(dt)
    if DEBUG_KEYS then
        for _, b in ipairs(DEBUG_BINDINGS) do
            if Input.GetKeyDown(b.key) then
                b.fn()
            end
        end
    end

    -- Drive phase-dependent UI off the GameMode's actual phase.
    local phase = Game.GetPhase()
    if phase == "Dead" and prevPhase ~= "Dead" then
        ShowDeath()
    elseif prevPhase == "Dead" and phase == "Playing" then
        -- Revive: fade in from the alpha death reached (no snap), over the same
        -- duration as the flourish so the screen and burst finish together.
        local a = Game.GetCameraFade()
        Game.CameraFade(a, 0, REVIVE_FLOURISH)
        HideDeath()
        ShowRevive()   -- play the revive burst over the fade-in
    elseif prevPhase == "Dead" and phase == "Defeated" then
        -- True death: keep the overlay (red icon on black) + return-to-title.
        ShowDefeated()
    end
    prevPhase = phase

    -- Redden the death icon in exact lockstep with the engine camera fade.
    -- Once fully faded to black, the death resolves into true death (defeat).
    if deathActive then
        local fade = Game.GetCameraFade()
        death:SetProperty("death-icon-red", "opacity", tostring(fade))
        if fade >= 1.0 then
            Game.Defeated()   -- Dead -> Defeated; next poll tears the overlay down
        end
    end

    -- Revive flourish: pop in at full alpha, then ease-out fade while dilating.
    if reviveActive then
        reviveElapsed = reviveElapsed + dt
        local t = reviveElapsed / REVIVE_FLOURISH
        if t >= 1 then
            HideRevive()
        else
            local e = 1 - (1 - t) * (1 - t)   -- ease-out: quick burst, gentle settle
            revive:SetProperty("revive-icon", "opacity", tostring(1 - e))
            revive:SetProperty("revive-icon", "transform", "scale(" .. (1 + REVIVE_GROWTH * e) .. ")")
        end
    end
end
