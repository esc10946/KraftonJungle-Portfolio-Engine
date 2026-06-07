local Context = require("FinalGameJamScript/Character/CharacterContext")
local Equipment = require("FinalGameJamScript/Character/CharacterEquipment")
local Locomotion = require("FinalGameJamScript/Character/CharacterLocomotion")
local Combat = require("FinalGameJamScript/Character/CharacterCombat")
local Guard = require("FinalGameJamScript/Character/CharacterGuard")
local Counter = require("FinalGameJamScript/Character/CharacterCounter")
local HitReaction = require("FinalGameJamScript/Character/CharacterHitReaction")
local PlayerGameState = require("FinalGameJamScript/Character/PlayerGameState")
local Commands = require("FinalGameJamScript/Character/CharacterCommands")

local ctx = nil

function BeginPlay()
    ctx = Context.Create(obj)

    Context.GetAnimInstance(ctx)
    Locomotion.BeginPlay(ctx)
    Equipment.BeginPlay(ctx)
    Combat.BeginPlay(ctx)
    Guard.BeginPlay(ctx)
    Counter.BeginPlay(ctx)
    HitReaction.BeginPlay(ctx)
end

function Tick(dt)
    if ctx == nil then
        return
    end

    if PlayerGameState.Tick(ctx, dt) then
        return
    end

    Commands.Tick(ctx, dt)
end

function OnDamaged(damageSpec, damageReport)
    if ctx == nil then
        return
    end

    if PlayerGameState.HandleDamaged(ctx, damageSpec, damageReport) then
        return
    end

    HitReaction.Play(ctx, damageSpec)
end
