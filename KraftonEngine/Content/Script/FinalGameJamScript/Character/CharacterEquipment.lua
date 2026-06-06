local EquipmentModule = {}

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
end

return EquipmentModule
