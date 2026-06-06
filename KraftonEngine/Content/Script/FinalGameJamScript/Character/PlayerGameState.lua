local Context = require("FinalGameJamScript/Character/CharacterContext")
local Locomotion = require("FinalGameJamScript/Character/CharacterLocomotion")
local State = require("FinalGameJamScript/Character/CharacterState")

local PlayerGameState = {}

local function should_end_game_from_damage_report(damageReport)
    if damageReport == nil then
        return false
    end

    if damageReport.bKilled == true then
        return true
    end

    return damageReport.NewHealth ~= nil and damageReport.NewHealth <= 0.0
end

local function request_player_death(ctx)
    if ctx.state.playerDead or ctx.state.playerDefeated then
        return
    end

    ctx.state.playerDead = true
    Context.StopCurrentMontage(ctx)
    State.ResetCombat(ctx)
    Locomotion.Lock(ctx)

    if Game ~= nil and Game.PlayerDeath ~= nil then
        Game.PlayerDeath()
    end
end

local function reset_after_revive(ctx)
    ctx.state.playerDead = false
    State.ResetCombat(ctx)
    Locomotion.Unlock(ctx)
end

function PlayerGameState.IsSuspended(ctx)
    return ctx.state.playerDead or ctx.state.playerDefeated
end

function PlayerGameState.Tick(ctx, dt)
    if ctx.state.playerDefeated then
        return true
    end

    if ctx.state.playerDead then
        --Dead phase: suspend combat. Resume only when the game flow revives us--
        if Game ~= nil and Game.GetPhase ~= nil and Game.GetPhase() == "Playing" then
            reset_after_revive(ctx)
            return false
        end

        return true
    end

    return false
end

function PlayerGameState.HandleDamaged(ctx, damageSpec, damageReport)
    if damageReport ~= nil and damageReport.AppliedDamage ~= nil and damageReport.AppliedDamage <= 0.0 then
        return true
    end

    if should_end_game_from_damage_report(damageReport) then
        request_player_death(ctx)
        return true
    end

    return false
end

return PlayerGameState
