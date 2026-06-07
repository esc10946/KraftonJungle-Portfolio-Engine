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

local function get_object_name(value)
    if value == nil or value.GetName == nil then
        return "nil"
    end

    return value:GetName()
end

local function set_particle_spawn_scale(particle, scale)
    if particle == nil then
        return
    end

    if Particle ~= nil and Particle.SetRuntimeSpawnRateScale ~= nil then
        Particle.SetRuntimeSpawnRateScale(particle, scale)
    elseif particle.SetRuntimeSpawnRateScale ~= nil then
        particle:SetRuntimeSpawnRateScale(scale)
    end
end

local function get_parry_particle(ctx, socketName)
    if ctx.cache.parryParticles[socketName] ~= nil then
        return ctx.cache.parryParticles[socketName]
    end

    if Particle == nil or Particle.AttachSystemToSocket == nil then
        print("[CharacterGuard] parry particle failed: Particle.AttachSystemToSocket is not bound")
        return nil
    end

    local particle = Particle.AttachSystemToSocket(ctx.obj, ctx.config.PARRY_PARTICLE_PATH, socketName)
    if particle == nil then
        print("[CharacterGuard] parry particle failed: socket=" .. socketName .. " path=" .. ctx.config.PARRY_PARTICLE_PATH)
        return nil
    end

    print("[CharacterGuard] parry particle attached: socket=" .. socketName
        .. " component=" .. get_object_name(particle))
    ctx.cache.parryParticles[socketName] = particle
    return particle
end

local function prepare_parry_particles(ctx)
    --Attach parry particles once and replay them on success--
    local rparticle = get_parry_particle(ctx, ctx.config.RPARRY_PARTICLE_SOCKET)
    local lparticle = get_parry_particle(ctx, ctx.config.LPARRY_PARTICLE_SOCKET)

    if rparticle ~= nil then
        set_particle_spawn_scale(rparticle, 0.0)
        rparticle:Activate(true)
        rparticle:ResetParticles()
    end

    if lparticle ~= nil then
        set_particle_spawn_scale(lparticle, 0.0)
        lparticle:Activate(true)
        lparticle:ResetParticles()
    end
end

local function activate_parry_particle(ctx, socketName)
    local particle = get_parry_particle(ctx, socketName)
    if particle ~= nil and particle.Activate ~= nil then
        --Replay particle from socket--
        set_particle_spawn_scale(particle, 1.0)
        if particle.ResetParticles ~= nil then
            particle:ResetParticles()
        end
        particle:Activate(true)
        print("[CharacterGuard] parry particle activate: socket=" .. socketName
            .. " component=" .. get_object_name(particle))
        return
    end

    print("[CharacterGuard] parry particle activate failed: socket=" .. socketName)
end

local function deactivate_parry_particle(ctx, socketName)
    local particle = ctx.cache.parryParticles[socketName]
    if particle ~= nil then
        --Stop spawn and clear remaining particles--
        set_particle_spawn_scale(particle, 0.0)
        if particle.ResetParticles ~= nil then
            particle:ResetParticles()
        end
        print("[CharacterGuard] parry particle deactivate: socket=" .. socketName)
    end
end

local function play_success_parry_particles(ctx)
    print("[CharacterGuard] play success parry particles")
    activate_parry_particle(ctx, ctx.config.RPARRY_PARTICLE_SOCKET)
    activate_parry_particle(ctx, ctx.config.LPARRY_PARTICLE_SOCKET)
    ctx.state.parryParticleTimeRemaining = ctx.config.PARRY_PARTICLE_DURATION
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
    ctx.state.canCancelSuccessParry = false
    ctx.state.parryWindowOpen = false
    Locomotion.Lock(ctx)
    --Clear old EnableAttack flag--
    Context.ConsumeEnableAttack(ctx)
    play_success_parry_particles(ctx)
    anim:PlayMontage(ctx.cache.successParryMontage)
end

function Guard.StopSuccessParryForCancel(ctx)
    if not ctx.state.successParryPlaying or not ctx.state.canCancelSuccessParry then
        return false
    end

    Context.StopCurrentMontage(ctx)
    State.ResetGuard(ctx)
    Locomotion.Unlock(ctx)
    Context.ConsumeEnableAttack(ctx)
    return true
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

function Guard.UpdateSuccessParryCancelWindow(ctx)
    if not ctx.state.successParryPlaying or ctx.state.canCancelSuccessParry then
        return
    end

    if Context.ConsumeEnableAttack(ctx) then
        ctx.state.canCancelSuccessParry = true
        Locomotion.Unlock(ctx)
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

    if ctx.state.canCancelSuccessParry and Locomotion.IsMoveKeyDown(ctx) then
        Guard.StopSuccessParryForCancel(ctx)
        return
    end

    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or ctx.cache.successParryMontage == nil or not anim:IsMontagePlaying(ctx.cache.successParryMontage) then
        State.ResetGuard(ctx)
        Locomotion.Unlock(ctx)
    end
end

function Guard.UpdateParryParticles(ctx, dt)
    if ctx.state.parryParticleTimeRemaining <= 0.0 then
        return
    end

    ctx.state.parryParticleTimeRemaining = ctx.state.parryParticleTimeRemaining - dt
    if ctx.state.parryParticleTimeRemaining > 0.0 then
        return
    end

    deactivate_parry_particle(ctx, ctx.config.RPARRY_PARTICLE_SOCKET)
    deactivate_parry_particle(ctx, ctx.config.LPARRY_PARTICLE_SOCKET)
end

function Guard.BeginPlay(ctx)
    load_guard_montages(ctx)
    prepare_parry_particles(ctx)
end

function Guard.Tick(ctx, dt)
    Guard.UpdateParryWindow(ctx)
    Guard.UpdateSuccessParry(ctx)
    Guard.UpdateSuccessParryCancelWindow(ctx)
    Guard.UpdateDefenseSequence(ctx)
    Guard.UpdateSuccessParrySequence(ctx)
    Guard.UpdateParryParticles(ctx, dt)
end

function Guard.IsInputTriggered(ctx)
    return is_parry_pressed(ctx)
end

return Guard
