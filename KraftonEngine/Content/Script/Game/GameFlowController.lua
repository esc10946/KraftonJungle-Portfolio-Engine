-- ============================================================
-- GameFlowController.lua
-- Attach to a single controller actor placed in the GamePlay scene.
-- Owns macro game-flow input/UI wiring:
--   * ESC toggles pause through the GameMode (Game.TogglePause).
--   * On entering the Dead phase, fades the camera out and reddens the
--     death icon; REVIVE / GIVE IN resolve the phase.
--   * On entering the Victory phase, eases in a self-dimming overlay (UI-layer
--     dim, not the camera fade) with LEADERBOARD / RETURN TO TITLE.
-- Phase-driven UI is synced by polling Game.GetPhase() each Tick.
-- ============================================================

local PAUSE_ZORDER    = 20      -- above the gameplay HUD
local DEATH_ZORDER    = 20      -- pause and death are mutually exclusive phases
local REVIVE_ZORDER   = 20      -- revive flourish (brief, after death tears down)
local DEATH_FADE      = 2.5     -- seconds: camera fade-out + icon reddening
local GIVE_IN_FADE    = 0.4     -- seconds: GIVE IN rushes the fade to full black
local REVIVE_FLOURISH = 0.6     -- seconds: revive icon pop -> fade-out + dilate
local REVIVE_GROWTH   = 0.4     -- revive icon scales 1.0 -> 1.4 over the flourish
local VICTORY_ZORDER  = 20      -- terminal overlay, like pause/death
local VICTORY_INTRO   = 1.2     -- seconds: dim + panel fade-in + icon settle
local VICTORY_DIM     = 0.72    -- final alpha of the UI-layer dim sheet
local VICTORY_ICON_IN = 0.8     -- victory icon scales 0.8 -> 1.0 as it settles
local LB_ZORDER       = 30      -- leaderboard layers ABOVE victory/death (z 20)
local LB_ROWS         = 6       -- row cells defined in Leaderboard.rml
local LB_ALPHABET     = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

local pause   = nil             -- pause overlay, created hidden, shown while paused
local death   = nil             -- death overlay, created hidden, shown while Dead
local revive  = nil             -- revive flourish overlay, shown briefly on revive
local victory = nil             -- victory overlay, shown while Victory
local leaderboard = nil         -- leaderboard overlay, shown over victory/death

local deathActive    = false    -- death sequence currently running
local reviveActive   = false    -- revive flourish currently playing
local reviveElapsed  = 0        -- seconds since the revive flourish started
local victoryActive  = false    -- victory intro ease currently playing
local victoryElapsed = 0        -- seconds since the victory intro started
local prevPhase      = nil      -- last polled phase, for edge detection

local lbSubmitMode   = false    -- true => name entry shown, SUBMIT writes a record
local lbLetters      = { 1, 1, 1 } -- 1-based indices into LB_ALPHABET (AAA)
local lbPendingTime  = 0        -- run time captured when submit mode opened
local lbPendingRev   = 0        -- revive count captured when submit mode opened

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
    death:SetProperty("leaderboard_btn", "display", "none")
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
    death:SetProperty("leaderboard_btn", "display", "block")
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

-- Victory: a self-clocked ease-in (its own clock, no camera fade). The dim
-- sheet rises to VICTORY_DIM while the panel fades in and the icon settles
-- 0.8 -> 1.0, so the scene darkens but the icon brightens. Captures the mouse
-- for the LEADERBOARD / RETURN TO TITLE buttons.
local function ShowVictory()
    if victory == nil then return end
    -- Reset to the pre-intro state (matches the RCSS start values, so the Tick
    -- ramp picks up without a snap).
    victory:SetProperty("victory-dim", "opacity", "0")
    victory:SetProperty("victory-panel", "opacity", "0")
    victory:SetProperty("victory-icon", "transform", "scale(" .. VICTORY_ICON_IN .. ")")
    victory:SetWantsMouse(true)
    victory:AddToViewportZ(VICTORY_ZORDER)
    victoryActive  = true
    victoryElapsed = 0
end

local function HideVictory()
    if victory ~= nil then victory:RemoveFromParent() end
    victoryActive = false
end

-- ---- Leaderboard overlay -------------------------------------------------
-- A self-contained overlay layered above victory/death. Two modes: submit
-- (from Victory, writes a record via the C++ Leaderboard store) and view-only
-- (from Defeat). Name entry is an arcade initials picker (+/- per slot) since
-- the engine's RmlUi has no keyboard text path.

local function lbName()
    return LB_ALPHABET:sub(lbLetters[1], lbLetters[1])
        .. LB_ALPHABET:sub(lbLetters[2], lbLetters[2])
        .. LB_ALPHABET:sub(lbLetters[3], lbLetters[3])
end

local function lbRefreshLetters()
    if leaderboard == nil then return end
    for i = 1, 3 do
        leaderboard:SetText("ltr" .. (i - 1), LB_ALPHABET:sub(lbLetters[i], lbLetters[i]))
    end
end

-- Seconds -> "M:SS.cc" (e.g. 83.42 -> "1:23.42").
local function lbFormatTime(t)
    local m = math.floor(t / 60)
    local s = t - m * 60
    return string.format("%d:%05.2f", m, s)
end

local function lbPopulate()
    if leaderboard == nil then return end
    local entries = Leaderboard.GetEntries()
    for i = 1, LB_ROWS do
        local e = entries[i]
        if e ~= nil then
            leaderboard:SetText("row" .. i .. "_name", e.name)
            leaderboard:SetText("row" .. i .. "_time", lbFormatTime(e.time))
            leaderboard:SetText("row" .. i .. "_rev", tostring(e.revives))
        else
            leaderboard:SetText("row" .. i .. "_name", "---")
            leaderboard:SetText("row" .. i .. "_time", "")
            leaderboard:SetText("row" .. i .. "_rev", "")
        end
    end
end

-- submit: true => name entry + SUBMIT (Victory). false => view-only (Defeat).
local function ShowLeaderboard(submit)
    if leaderboard == nil then return end
    lbSubmitMode = submit
    -- AddToViewport loads the RML document; SetText/SetProperty no-op until then,
    -- so add FIRST, then populate.
    leaderboard:SetWantsMouse(true)
    leaderboard:AddToViewportZ(LB_ZORDER)
    lbPopulate()
    if submit then
        lbLetters = { 1, 1, 1 }
        lbRefreshLetters()
        lbPendingTime = Game.GetActiveTime()
        lbPendingRev  = Game.GetReviveCount()
        -- SetText feeds SetInnerRML, so the spans are parsed: scale the YOZAKURA
        -- numerals (.lb-num) up to match the Serpentine words around them.
        -- Explicit spacer span between the two stats: RmlUi collapses runs of
        -- literal whitespace to a single space, so padding with "   " won't gap them.
        leaderboard:SetText("lb-yourtime",
            "YOUR TIME  <span class='lb-num'>" .. lbFormatTime(lbPendingTime) .. "</span>"
            .. "<span class='lb-gap'></span>"
            .. "<span class='lb-num'>" .. lbPendingRev .. "</span> revives")
        leaderboard:SetProperty("lb-entry", "display", "block")
    else
        leaderboard:SetProperty("lb-entry", "display", "none")
    end
end

local function HideLeaderboard()
    if leaderboard ~= nil then leaderboard:RemoveFromParent() end
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
        -- View-only after a defeat: no record is written from the death screen.
        death:bind_click("leaderboard_btn", function() ShowLeaderboard(false) end)
        death:bind_click("title_btn", function() Game.QuitToTitle() end)
    end

    revive = UI.CreateWidget("Content/Game/UI/Revive.rml")
    if revive == nil then
        print("[GameFlowController] failed to create Revive.rml widget")
    end

    victory = UI.CreateWidget("Content/Game/UI/Victory.rml")
    if victory == nil then
        print("[GameFlowController] failed to create Victory.rml widget")
    else
        -- LEADERBOARD opens the overlay in submit mode (the run is frozen in the
        -- Victory phase, so its time/revives are final). It layers above this
        -- panel without changing phase, so closing it returns here.
        victory:bind_click("leaderboard_btn", function() ShowLeaderboard(true) end)
        victory:bind_click("title_btn", function() Game.QuitToTitle() end)
    end

    leaderboard = UI.CreateWidget("Content/Game/UI/Leaderboard.rml")
    if leaderboard == nil then
        print("[GameFlowController] failed to create Leaderboard.rml widget")
    else
        -- Initials picker: +/- cycles each slot's letter A-Z (wrapping).
        for i = 1, 3 do
            local slot = i
            leaderboard:bind_click("up" .. (i - 1), function()
                lbLetters[slot] = (lbLetters[slot] % 26) + 1
                lbRefreshLetters()
            end)
            leaderboard:bind_click("down" .. (i - 1), function()
                lbLetters[slot] = ((lbLetters[slot] - 2) % 26) + 1
                lbRefreshLetters()
            end)
        end
        leaderboard:bind_click("submit_btn", function()
            if not lbSubmitMode then return end   -- guard: view-only mode
            Leaderboard.Submit(lbName(), lbPendingTime, lbPendingRev)
            lbSubmitMode = false
            leaderboard:SetProperty("lb-entry", "display", "none")
            lbPopulate()   -- refresh so the new record shows in rank order
        end)
        leaderboard:bind_click("close_btn", function() HideLeaderboard() end)
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
    if victory ~= nil then
        victory:RemoveFromParent()
        victory = nil
    end
    if leaderboard ~= nil then
        leaderboard:RemoveFromParent()
        leaderboard = nil
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
    elseif phase == "Victory" and prevPhase ~= "Victory" then
        ShowVictory()
    elseif prevPhase == "Victory" and phase ~= "Victory" then
        -- Left Victory (e.g. LEADERBOARD -> Leaderboard phase): tear it down so
        -- the next screen takes over.
        HideVictory()
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

    -- Victory intro: ease the dim up, fade the panel in, settle the icon. Same
    -- ease-out curve as the revive flourish; holds at full once it completes.
    if victoryActive then
        victoryElapsed = victoryElapsed + dt
        local t = victoryElapsed / VICTORY_INTRO
        if t >= 1 then
            victory:SetProperty("victory-dim", "opacity", tostring(VICTORY_DIM))
            victory:SetProperty("victory-panel", "opacity", "1")
            victory:SetProperty("victory-icon", "transform", "scale(1.0)")
            victoryActive = false
        else
            local e = 1 - (1 - t) * (1 - t)   -- ease-out: quick rise, gentle settle
            victory:SetProperty("victory-dim", "opacity", tostring(VICTORY_DIM * e))
            victory:SetProperty("victory-panel", "opacity", tostring(e))
            victory:SetProperty("victory-icon", "transform", "scale(" .. (VICTORY_ICON_IN + (1.0 - VICTORY_ICON_IN) * e) .. ")")
        end
    end
end
