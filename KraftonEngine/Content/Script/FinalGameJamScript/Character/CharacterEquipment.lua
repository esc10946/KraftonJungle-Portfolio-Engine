local EquipmentModule = {}

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

local function get_object_name(value)
    if value == nil or value.GetName == nil then
        return "nil"
    end

    return value:GetName()
end

local function attach_trail_particle(ctx, socketName)
    if Particle == nil or Particle.AttachSystemToSocket == nil then
        if not ctx.equipment.trailBindingWarned then
            --If this prints, rebuild C++ first--
            print("[CharacterController] Particle.AttachSystemToSocket binding missing")
            ctx.equipment.trailBindingWarned = true
        end
        return nil
    end

    local particle = Particle.AttachSystemToSocket(ctx.obj, ctx.config.SWORD_TRAIL_PARTICLE_PATH, socketName)
    if particle == nil then
        print("[CharacterController] sword trail attach failed: socket=" .. socketName
            .. " path=" .. ctx.config.SWORD_TRAIL_PARTICLE_PATH)
        return nil
    end

    --Keep sword trail ready, but do not spawn until attack--
    set_particle_spawn_scale(particle, 0.0)
    if particle.Activate ~= nil then
        particle:Activate(true)
    end
    if particle.ResetParticles ~= nil then
        particle:ResetParticles()
    end

    print("[CharacterController] sword trail attached: socket=" .. socketName
        .. " component=" .. get_object_name(particle))
    return particle
end

local function set_trail_particle_active(particle, active)
    if particle == nil then
        return
    end

    if active then
        --Clear old trail and spawn from this attack--
        if particle.ResetParticles ~= nil then
            particle:ResetParticles()
        end
        set_particle_spawn_scale(particle, 1.0)
        if particle.Activate ~= nil then
            particle:Activate(true)
        end
        return
    end

    --Stop spawning and clear remaining trail--
    set_particle_spawn_scale(particle, 0.0)
    if particle.ResetParticles ~= nil then
        particle:ResetParticles()
    end
    if particle.Deactivate ~= nil then
        particle:Deactivate()
    end
end

function EquipmentModule.BeginPlay(ctx)
    if ctx.equipment.RweaponComponent ~= nil or ctx.obj == nil then
        return
    end
    if ctx.equipment.LweaponComponent ~= nil or ctx.obj == nil then
        return
    end

    if Equipment == nil or Equipment.AttachStaticMeshToSocket == nil then
        if not ctx.equipment.weaponBindingWarned then
            --If this prints, rebuild C++ first--
            print("[CharacterController] Equipment.AttachStaticMeshToSocket binding missing")
            ctx.equipment.weaponBindingWarned = true
        end
        return
    end

    --Create weapon component and attach to hand socket--
    ctx.equipment.LweaponComponent = Equipment.AttachStaticMeshToSocket(ctx.obj, ctx.config.WEAPON_MESH_PATH, ctx.config.LWEAPON_SOCKET, Vec3(1.0,1.0,1.0))
    ctx.equipment.RweaponComponent = Equipment.AttachStaticMeshToSocket(ctx.obj, ctx.config.WEAPON_MESH_PATH, ctx.config.RWEAPON_SOCKET, Vec3(1.0,1.0,1.0))

    --Create sword trail particles without touching counter particles--
    ctx.equipment.LtrailParticle = attach_trail_particle(ctx, ctx.config.LSWORD_TRAIL_PARTICLE_SOCKET)
    ctx.equipment.RtrailParticle = attach_trail_particle(ctx, ctx.config.RSWORD_TRAIL_PARTICLE_SOCKET)
end

function EquipmentModule.ActivateTrail(ctx)
    ctx.equipment.trailActive = true
    set_trail_particle_active(ctx.equipment.LtrailParticle, true)
    set_trail_particle_active(ctx.equipment.RtrailParticle, true)
end

function EquipmentModule.DeactivateTrail(ctx)
    ctx.equipment.trailActive = false
    set_trail_particle_active(ctx.equipment.LtrailParticle, false)
    set_trail_particle_active(ctx.equipment.RtrailParticle, false)
end

return EquipmentModule
