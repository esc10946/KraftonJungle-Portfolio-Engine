local Context = require("FinalGameJamScript/Character/CharacterContext")

local LockOn = {}

local function get_lock_on_component(ctx)
    if ctx.cache.lockOnComponent ~= nil then
        return ctx.cache.lockOnComponent
    end

    local owner = ctx.obj
    if owner == nil then
        return nil
    end

    if owner.GetLockOnComponent ~= nil then
        ctx.cache.lockOnComponent = owner:GetLockOnComponent()
        return ctx.cache.lockOnComponent
    end

    ctx.cache.lockOnComponent = Context.Call(owner, "GetLockOnComponent")
    return ctx.cache.lockOnComponent
end

local function is_lock_on_pressed(ctx)
    return Input.GetKeyDown(ctx.config.LOCK_ON)
end

function LockOn.BeginPlay(ctx)
    get_lock_on_component(ctx)
end

function LockOn.ExecuteInput(ctx)
    local lockOn = get_lock_on_component(ctx)
    if lockOn == nil then
        return
    end

    Context.Call(lockOn, "ToggleLockOn")
end

function LockOn.UpdateSwitchInput(ctx)
    local lockOn = get_lock_on_component(ctx)
    if lockOn == nil then
        return
    end

    local mouseDeltaX = Input.GetMouseDeltaX()
    if mouseDeltaX == nil or mouseDeltaX == 0.0 then
        return
    end

    Context.Call(lockOn, "AddSwitchInput", mouseDeltaX)
end

function LockOn.IsInputTriggered(ctx)
    return is_lock_on_pressed(ctx)
end

return LockOn
