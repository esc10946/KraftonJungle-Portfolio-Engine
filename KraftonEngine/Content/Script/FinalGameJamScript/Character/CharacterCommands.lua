local Combat = require("FinalGameJamScript/Character/CharacterCombat")
local Guard = require("FinalGameJamScript/Character/CharacterGuard")
local HitReaction = require("FinalGameJamScript/Character/CharacterHitReaction")
local Locomotion = require("FinalGameJamScript/Character/CharacterLocomotion")

local Commands = {}

local function run_command(ctx, command, dt)
    if command.Tick ~= nil then
        command.Tick(ctx, dt)
    end

    if command.Execute == nil then
        return
    end

    if command.IsTriggered ~= nil and not command.IsTriggered(ctx) then
        return
    end

    if command.CanExecute ~= nil and not command.CanExecute(ctx) then
        return
    end

    command.Execute(ctx, dt)
end

local commandList = {
    { Tick = Combat.UpdateAttackChainWindow },
    { Tick = HitReaction.UpdateCancelWindow },
    { Tick = Guard.UpdateParryWindow },
    { Tick = Guard.UpdateParryParticles },
    { Tick = Guard.UpdateSuccessParry },
    { Tick = Guard.UpdateSuccessParryCancelWindow },
    { Tick = Locomotion.UpdateMovementLock },
    {
        IsTriggered = Guard.IsInputTriggered,
        Execute = Guard.ExecuteInput,
    },
    --Attack input comes before movement cancel to keep combo while holding move key--
    {
        IsTriggered = Combat.IsInputTriggered,
        Execute = Combat.ExecuteInput,
    },
    {
        CanExecute = function(ctx)
            return ctx.state.attackInputQueued and ctx.state.attackPlaying and ctx.state.canChainAttack
        end,
        Execute = Combat.TryPlayQueuedAttack,
    },
    {
        IsTriggered = Locomotion.IsMoveKeyDown,
        CanExecute = function(ctx)
            return ctx.state.attackPlaying and ctx.state.canChainAttack
        end,
        Execute = Combat.StopAttackForMovement,
    },
    { Tick = Locomotion.UpdateGroundSpeed },
    { Tick = Combat.UpdateAttackSequence },
    {
        Tick = function(ctx, dt)
            Guard.UpdateDefenseSequence(ctx)
            Guard.UpdateSuccessParrySequence(ctx)
        end,
    },
    { Tick = HitReaction.UpdateSequence },
}

function Commands.Tick(ctx, dt)
    for _, command in ipairs(commandList) do
        run_command(ctx, command, dt)
    end
end

return Commands
