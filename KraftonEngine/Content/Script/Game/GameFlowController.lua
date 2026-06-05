-- ============================================================
-- GameFlowController.lua
-- Attach to a single controller actor placed in the GamePlay scene.
-- Owns macro game-flow input/UI wiring. Currently: ESC toggles pause
-- through the GameMode (Game.TogglePause) and shows the pause overlay.
-- ============================================================

local PAUSE_ZORDER = 20         -- above the gameplay HUD

local pause = nil               -- pause overlay, created hidden, shown while paused

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
end

function Tick(dt)
end
