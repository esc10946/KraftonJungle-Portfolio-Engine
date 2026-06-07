local Context = require("FinalGameJamScript/Character/CharacterContext")
local Locomotion = require("FinalGameJamScript/Character/CharacterLocomotion")
local State = require("FinalGameJamScript/Character/CharacterState")

local Counter = {}

local function atan2_degrees(y, x)
    if math.atan2 ~= nil then
        return math.deg(math.atan2(y, x))
    end

    if x > 0.0 then
        return math.deg(math.atan(y / x))
    end

    if x < 0.0 and y >= 0.0 then
        return math.deg(math.atan(y / x)) + 180.0
    end

    if x < 0.0 and y < 0.0 then
        return math.deg(math.atan(y / x)) - 180.0
    end

    if y > 0.0 then
        return 90.0
    end

    if y < 0.0 then
        return -90.0
    end

    return 0.0
end

local function smooth_step(alpha)
    alpha = math.max(0.0, math.min(1.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)
end

local function lerp_vec(a, b, alpha)
    return a + (b - a) * alpha
end

local function load_counter_montage(ctx)
    if ctx.cache.counterMontage == nil then
        ctx.cache.counterMontage = Context.LoadMontage(ctx.config.COUNTER_MONTAGE_PATH)
    end
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

local function activate_counter_impact_particle(ctx, hitLocation)
    if hitLocation == nil or Particle == nil then
        return
    end

    local particle = ctx.cache.counterImpactParticle
    if particle == nil and Particle.SpawnSystemAtLocation ~= nil then
        particle = Particle.SpawnSystemAtLocation(ctx.obj, ctx.config.COUNTER_PARTICLE_PATH, hitLocation)
        ctx.cache.counterImpactParticle = particle
    end

    if particle == nil then
        print("[CharacterCounter] impact particle spawn failed")
        return
    end

    if Particle.SetSystemWorldLocation ~= nil then
        Particle.SetSystemWorldLocation(particle, hitLocation)
    end
    if Particle.SetSystemWorldScale ~= nil then
        Particle.SetSystemWorldScale(particle, ctx.config.COUNTER_IMPACT_PARTICLE_SCALE)
    end
    set_particle_spawn_scale(particle, 1.0)
    if particle.ResetParticles ~= nil then
        particle:ResetParticles()
    end
    if particle.Activate ~= nil then
        particle:Activate(true)
    end
end

local function deactivate_counter_impact_particle(ctx)
    local particle = ctx.cache.counterImpactParticle
    if particle == nil then
        return
    end

    set_particle_spawn_scale(particle, 0.0)
    if particle.ResetParticles ~= nil then
        particle:ResetParticles()
    end
end

local function play_counter_particles(ctx, hitLocation)
    print("[CharacterCounter] play counter particles")
    activate_counter_impact_particle(ctx, hitLocation)
    ctx.state.counterParticleTimeRemaining = ctx.config.COUNTER_PARTICLE_DURATION
end

local function play_counter_slomo(ctx)
    local action = Context.GetActionComponent(ctx)
    if action == nil or action.Slomo == nil then
        print("[Slomo] : skipped, ActionComponent or Slomo binding missing")
        return
    end

    --Slow world briefly on counter success--
    print("[Slomo] : play duration=" .. string.format("%.3f", ctx.config.COUNTER_SLOMO_DURATION)
        .. " dilation=" .. string.format("%.3f", ctx.config.COUNTER_SLOMO_TIME_DILATION))
    action:Slomo(ctx.config.COUNTER_SLOMO_DURATION, ctx.config.COUNTER_SLOMO_TIME_DILATION)
end

local function get_root_primitive(ctx)
    if ctx.obj == nil or ctx.obj.GetRootPrimitiveComponent == nil then
        return nil
    end

    local primitive = ctx.obj:GetRootPrimitiveComponent()
    if primitive ~= nil and primitive.IsValid ~= nil and not primitive:IsValid() then
        return nil
    end

    return primitive
end

local function begin_pawn_overlap(ctx)
    if ctx.state.counterCollision ~= nil then
        return
    end

    local primitive = get_root_primitive(ctx)
    if primitive == nil or primitive.GetPawnCollisionResponse == nil or primitive.SetPawnCollisionResponse == nil then
        return
    end

    local originalResponse = primitive:GetPawnCollisionResponse()
    ctx.state.counterCollision = {
        primitive = primitive,
        pawnResponse = originalResponse,
    }

    --Let the counter reposition pass through Pawn collision, then restore it on counter end--
    primitive:SetPawnCollisionResponse(ctx.config.COUNTER_PAWN_OVERLAP_RESPONSE or 1)
end

local function restore_pawn_overlap(ctx)
    local collision = ctx.state.counterCollision
    if collision == nil then
        return
    end

    local primitive = collision.primitive
    if primitive ~= nil and primitive.SetPawnCollisionResponse ~= nil then
        if primitive.IsValid == nil or primitive:IsValid() then
            primitive:SetPawnCollisionResponse(collision.pawnResponse)
        end
    end

    ctx.state.counterCollision = nil
end

function Counter.RestoreCollision(ctx)
    restore_pawn_overlap(ctx)
end

function Counter.Reset(ctx)
    restore_pawn_overlap(ctx)
    State.ResetCounter(ctx)
end

local function face_target(ctx, target)
    if ctx.obj == nil or ctx.obj.Location == nil or target == nil or target.Location == nil then
        return
    end

    local direction = target.Location - ctx.obj.Location
    direction = Context.NormalizeFlatDirection(direction)
    if direction == nil then
        return
    end

    ctx.obj.Rotation = Vec3(0.0, 0.0, atan2_degrees(direction.Y, direction.X))
end

local function start_reposition_behind_target(ctx, target)
    if not ctx.config.COUNTER_REPOSITION_ENABLED then
        return
    end

    if ctx.obj == nil or ctx.obj.Location == nil or target == nil or target.Location == nil then
        return
    end

    local targetForward = Context.NormalizeFlatDirection(target.Forward)
    if targetForward == nil then
        local fromTargetToPlayer = ctx.obj.Location - target.Location
        targetForward = Context.NormalizeFlatDirection(fromTargetToPlayer)
    end

    if targetForward == nil then
        return
    end

    local distance = ctx.config.COUNTER_REPOSITION_DISTANCE or 120.0
    local duration = ctx.config.COUNTER_REPOSITION_DURATION or 0.18
    local startLocation = ctx.obj.Location
    local targetLocation = target.Location - targetForward * distance
    targetLocation.Z = startLocation.Z

    --Move player behind the attacker while the counter montage starts--
    ctx.state.counterMove = {
        target = target,
        startLocation = startLocation,
        targetLocation = targetLocation,
        elapsed = 0.0,
        duration = math.max(0.01, duration),
    }
end

local function update_reposition(ctx, dt)
    local move = ctx.state.counterMove
    if move == nil then
        return
    end

    if ctx.obj == nil or ctx.obj.Location == nil then
        ctx.state.counterMove = nil
        return
    end

    move.elapsed = move.elapsed + dt
    local alpha = smooth_step(move.elapsed / move.duration)
    ctx.obj.Location = lerp_vec(move.startLocation, move.targetLocation, alpha)

    if move.target ~= nil then
        face_target(ctx, move.target)
    end

    if move.elapsed >= move.duration then
        ctx.obj.Location = move.targetLocation
        ctx.state.counterMove = nil
    end
end

function Counter.Play(ctx, target, hitLocation)
    load_counter_montage(ctx)

    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or ctx.cache.counterMontage == nil then
        State.ResetGuard(ctx)
        Counter.Reset(ctx)
        Locomotion.Unlock(ctx)
        return
    end

    Context.StopCurrentMontage(ctx)
    State.ResetGuard(ctx)
    ctx.state.counterPlaying = true
    ctx.state.canCancelCounter = false
    Locomotion.Lock(ctx)
    --Clear old EnableAttack flag--
    Context.ConsumeEnableAttack(ctx)
    play_counter_particles(ctx, hitLocation)
    play_counter_slomo(ctx)
    begin_pawn_overlap(ctx)
    start_reposition_behind_target(ctx, target)
    face_target(ctx, target)
    anim:PlayMontage(ctx.cache.counterMontage, ctx.config.COUNTER_SECTION)
end

function Counter.StopForCancel(ctx)
    if not ctx.state.counterPlaying or not ctx.state.canCancelCounter then
        return false
    end

    Context.StopCurrentMontage(ctx)
    Counter.Reset(ctx)
    Locomotion.Unlock(ctx)
    Context.ConsumeEnableAttack(ctx)
    return true
end

function Counter.UpdateOpportunity(ctx)
    if not ctx.state.defensePlaying then
        return
    end

    local opportunityTarget, hitLocation = Context.ConsumeCounterOpportunity(ctx)
    if opportunityTarget ~= nil then
        if opportunityTarget == true then
            Counter.Play(ctx, nil, hitLocation)
        else
            Counter.Play(ctx, opportunityTarget, hitLocation)
        end
    end
end

function Counter.UpdateCancelWindow(ctx)
    if not ctx.state.counterPlaying or ctx.state.canCancelCounter then
        return
    end

    if Context.ConsumeEnableAttack(ctx) then
        ctx.state.canCancelCounter = true
        Locomotion.Unlock(ctx)
    end
end

function Counter.UpdateSequence(ctx, dt)
    if not ctx.state.counterPlaying then
        return
    end

    update_reposition(ctx, dt or 0.0)

    if ctx.state.canCancelCounter and Locomotion.IsMoveKeyDown(ctx) then
        Counter.StopForCancel(ctx)
        return
    end

    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or ctx.cache.counterMontage == nil or not anim:IsMontagePlaying(ctx.cache.counterMontage) then
        Counter.Reset(ctx)
        Locomotion.Unlock(ctx)
    end
end

function Counter.UpdateParticles(ctx, dt)
    if ctx.state.counterParticleTimeRemaining <= 0.0 then
        return
    end

    ctx.state.counterParticleTimeRemaining = ctx.state.counterParticleTimeRemaining - dt
    if ctx.state.counterParticleTimeRemaining > 0.0 then
        return
    end

    deactivate_counter_impact_particle(ctx)
end

function Counter.BeginPlay(ctx)
    load_counter_montage(ctx)
end

function Counter.Tick(ctx, dt)
    Counter.UpdateOpportunity(ctx)
    Counter.UpdateCancelWindow(ctx)
    Counter.UpdateSequence(ctx, dt)
    Counter.UpdateParticles(ctx, dt)
end

return Counter
