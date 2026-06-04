local widget
function BeginPlay()
    widget = UI.CreateWidget("UI/Title.rml")
    widget:SetWantsMouse(true)          -- show cursor, release mouse-look
    widget:AddToViewport()
    widget:bind_click("start_btn", function() Engine.OpenScene("DoF_Demo") end)
    widget:bind_click("quit_btn",  function() Engine.Exit() end)
end
function EndPlay() if widget then widget:RemoveFromParent() end end