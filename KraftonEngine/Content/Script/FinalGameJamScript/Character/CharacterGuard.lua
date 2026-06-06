local Context = require("FinalGameJamScript/Character/CharacterContext")
local Locomotion = require("FinalGameJamScript/Character/CharacterLocomotion")
local State = require("FinalGameJamScript/Character/CharacterState")
local HitReaction = require("FinalGameJamScript/Character/CharacterHitReaction")

local Guard = {}

local function load_guard_montages(ctx)
    if ctx.cache.defenseIdleMontage == nil then
        ctx.cache.defenseIdleMontage = Context.LoadMontage(ctx.config.DEFENSE_IDLE_MONTAGE_PATH)
    end

    if ctx.cache.successParryMontage == nil then
        ctx.cache.successParryMontage = Context.LoadMontage(ctx.config.SUCCESS_PARRY_MONTAGE_PATH)
    end
end

local function is_parry_pressed(ctx)
    return Input.GetKeyDown(ctx.config.RMB)
end

local function is_parry_down(ctx)
    return Input.GetKey(ctx.config.RMB)
end

local function is_parry_released(ctx)
    return Input.GetKeyUp(ctx.config.RMB)
end

function Guard.StopDefense(ctx)
    if not ctx.state.defensePlaying then
        return
    end

    Context.StopCurrentMontage(ctx)
    ctx.state.defensePlaying = false
    ctx.state.defenseIdlePlaying = false
    ctx.state.parryWindowOpen = false
    Locomotion.Unlock(ctx)
end

function Guard.PlayDefenseIdle(ctx)
    if ctx.state.defenseIdlePlaying then
        return
    end

    load_guard_montages(ctx)

    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or ctx.cache.defenseIdleMontage == nil then
        State.ResetGuard(ctx)
        return
    end

    Context.StopCurrentMontage(ctx)
    State.ResetAttack(ctx)
    Context.FaceCameraForward(ctx)

    ctx.state.defensePlaying = true
    ctx.state.defenseIdlePlaying = true
    ctx.state.successParryPlaying = false
    ctx.state.parryWindowOpen = false

    Locomotion.Lock(ctx)
    anim:PlayMontage(ctx.cache.defenseIdleMontage)
end

function Guard.PlaySuccessParry(ctx)
    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or ctx.cache.successParryMontage == nil then
        State.ResetGuard(ctx)
        Locomotion.Unlock(ctx)
        return
    end

    Context.StopCurrentMontage(ctx)
    ctx.state.defensePlaying = false
    ctx.state.defenseIdlePlaying = false
    ctx.state.successParryPlaying = true
    ctx.state.parryWindowOpen = false
    Locomotion.Lock(ctx)
    anim:PlayMontage(ctx.cache.successParryMontage)
end

function Guard.ExecuteInput(ctx)
    if ctx.state.attackPlaying or ctx.state.defensePlaying or ctx.state.successParryPlaying then
        return
    end

    if ctx.state.hitPlaying then
        if not ctx.state.canCancelHit then
            return
        end

        --EnableAttack Notify opens guard cancel from hit--
        HitReaction.StopForCancel(ctx)
    end

    if ctx.state.playerDefeated then
        return
    end

    Guard.PlayDefenseIdle(ctx)
end

function Guard.UpdateParryWindow(ctx)
    if not ctx.state.defensePlaying then
        ctx.state.parryWindowOpen = false
        return
    end

    ctx.state.parryWindowOpen = Context.IsParryWindowActive(ctx)
end

function Guard.UpdateSuccessParry(ctx)
    if not ctx.state.defensePlaying then
        return
    end

    if Context.ConsumeSuccessfulParry(ctx) then
        Guard.PlaySuccessParry(ctx)
    end
end

function Guard.UpdateDefenseSequence(ctx)
    if not ctx.state.defensePlaying then
        return
    end

    if is_parry_released(ctx) then
        Guard.StopDefense(ctx)
        return
    end

    local anim = Context.GetAnimInstance(ctx)
    if anim == nil then
        State.ResetGuard(ctx)
        Locomotion.Unlock(ctx)
        return
    end

    if ctx.state.defenseIdlePlaying
        and ctx.cache.defenseIdleMontage ~= nil
        and not anim:IsMontagePlaying(ctx.cache.defenseIdleMontage) then
        if is_parry_down(ctx) then
            ctx.state.defenseIdlePlaying = false
            Guard.PlayDefenseIdle(ctx)
            return
        end

        Guard.StopDefense(ctx)
    end
end

function Guard.UpdateSuccessParrySequence(ctx)
    if not ctx.state.successParryPlaying then
        return
    end

    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or ctx.cache.successParryMontage == nil or not anim:IsMontagePlaying(ctx.cache.successParryMontage) then
        ctx.state.successParryPlaying = false
        Locomotion.Unlock(ctx)
    end
end

function Guard.BeginPlay(ctx)
    load_guard_montages(ctx)
end

function Guard.Tick(ctx, dt)
    Guard.UpdateParryWindow(ctx)
    Guard.UpdateSuccessParry(ctx)
    Guard.UpdateDefenseSequence(ctx)
    Guard.UpdateSuccessParrySequence(ctx)
end

function Guard.IsInputTriggered(ctx)
    return is_parry_pressed(ctx)
end

return Guard
