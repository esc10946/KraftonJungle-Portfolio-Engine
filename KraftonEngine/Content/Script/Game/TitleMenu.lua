-- ============================================================
-- TitleMenu.lua
-- Attach to a single controller actor placed in the Title scene.
-- On BeginPlay it builds the title menu (Title.rml) and wires the
-- four buttons:
--
--   start_btn   -> transition to the gameplay scene
--   options_btn -> open Options screen      (TODO)
--   credits_btn -> transition to the credits scene
--   exit_btn    -> quit the application
-- ============================================================

local START_SCENE   = "Game/GamePlay"
local CREDITS_SCENE = "Game/GameCredits"

local widget = nil          -- title menu

function BeginPlay()
    widget = UI.CreateWidget("Content/Game/UI/Title.rml")
    if widget == nil then
        print("[TitleMenu] failed to create Title.rml widget")
        return
    end

    -- Menu needs the cursor: show system cursor, release raw mouse / mouse-look.
    widget:SetWantsMouse(true)
    widget:AddToViewport()

    widget:bind_click("start_btn", function()
        Engine.OpenScene(START_SCENE)
    end)

    widget:bind_click("options_btn", function()
        -- TODO: open Options screen
        print("[TitleMenu] Options clicked")
    end)

    widget:bind_click("credits_btn", function()
        Engine.OpenScene(CREDITS_SCENE)
    end)

    widget:bind_click("exit_btn", function()
        Engine.Exit()
    end)
end

function EndPlay()
    if widget ~= nil then
        widget:RemoveFromParent()
        widget = nil
    end
end

function Tick(dt)
end
