local Config = require("FinalGameJamScript/Character/CharacterConfig")

local Context = {}

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

local function call_function(target, functionName, ...)
    if target == nil then
        return nil
    end

    local directFunction = target[functionName]
    if directFunction ~= nil then
        return directFunction(target, ...)
    end

    if target.CallFunction ~= nil then
        return target:CallFunction(functionName, ...)
    end

    return nil
end

function Context.Create(owner)
    return {
        obj = owner,
        config = Config,
        state = {
            currentAttackIndex = 0,
            attackPlaying = false,
            canChainAttack = false,
            attackInputQueued = false,
            defensePlaying = false,
            defenseIdlePlaying = false,
            successParryPlaying = false,
            hitPlaying = false,
            currentHitMontage = nil,
            canCancelHit = false,
            playerDead = false,
            playerDefeated = false,
            parryWindowOpen = false,
            movementLocked = false,
        },
        cache = {
            animInstance = nil,
            movement = nil,
            attackMontages = {},
            hitMontages = {},
            defenseIdleMontage = nil,
            successParryMontage = nil,
        },
        equipment = {
            RweaponComponent = nil,
            LweaponComponent = nil,
            weaponBindingWarned = false,
        },
    }
end

function Context.Call(target, functionName, ...)
    return call_function(target, functionName, ...)
end

function Context.GetAnimInstance(ctx)
    if ctx.cache.animInstance ~= nil or ctx.obj == nil or ctx.obj.GetSkeletalMeshComponent == nil then
        return ctx.cache.animInstance
    end

    local mesh = ctx.obj:GetSkeletalMeshComponent()
    if mesh == nil then
        return nil
    end

    ctx.cache.animInstance = mesh:GetAnimInstance()
    return ctx.cache.animInstance
end

function Context.GetCharacterMovement(ctx)
    if ctx.cache.movement ~= nil then
        return ctx.cache.movement
    end

    local owner = ctx.obj
    if owner == nil then
        return nil
    end

    --obj is bound as Actor, so ACharacter functions can be hidden--
    if owner.GetCharacterMovement ~= nil then
        ctx.cache.movement = owner:GetCharacterMovement()
        return ctx.cache.movement
    end

    --Call reflected ACharacter function when obj's lua type is Actor--
    if owner.IsA ~= nil and owner.CallFunction ~= nil and owner:IsA("ACharacter") then
        ctx.cache.movement = owner:CallFunction("GetCharacterMovement")
        return ctx.cache.movement
    end

    return nil
end

function Context.StopCurrentMontage(ctx)
    local anim = Context.GetAnimInstance(ctx)
    if anim ~= nil and anim.StopMontage ~= nil then
        --Stop montage immediately--
        anim:StopMontage(0.0)
    end
end

function Context.ConsumeEnableAttack(ctx)
    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or Animation == nil or Animation.ConsumeEnableAttack == nil then
        return false
    end

    return Animation.ConsumeEnableAttack(anim)
end

function Context.IsParryWindowActive(ctx)
    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or Animation == nil or Animation.IsParryWindowActive == nil then
        return false
    end

    return Animation.IsParryWindowActive(anim)
end

function Context.ConsumeSuccessfulParry(ctx)
    local anim = Context.GetAnimInstance(ctx)
    if anim == nil or Animation == nil or Animation.ConsumeSuccessfulParry == nil then
        return false
    end

    return Animation.ConsumeSuccessfulParry(anim)
end

function Context.LoadMontage(path)
    if Animation == nil or Animation.LoadMontage == nil then
        return nil
    end

    return Animation.LoadMontage(path)
end

function Context.GetActiveCamera()
    if CameraManager == nil then
        return nil
    end

    if CameraManager.GetPossessedCamera ~= nil then
        local possessedCamera = CameraManager.GetPossessedCamera()
        if possessedCamera ~= nil then
            return possessedCamera
        end
    end

    if CameraManager.GetActiveCamera ~= nil then
        return CameraManager.GetActiveCamera()
    end

    return nil
end

function Context.GetCameraFlatForward()
    local camera = Context.GetActiveCamera()
    if camera == nil or camera.Forward == nil then
        return nil
    end

    local forward = camera.Forward
    forward.Z = 0.0

    if forward:Length() <= 0.0001 then
        return nil
    end

    return forward:Normalized()
end

function Context.FaceCameraForward(ctx)
    if ctx.obj == nil then
        return
    end

    local forward = Context.GetCameraFlatForward()
    if forward == nil then
        return
    end

    local yaw = atan2_degrees(forward.Y, forward.X)
    --FVector rotation uses X=Roll, Y=Pitch, Z=Yaw--
    ctx.obj.Rotation = Vec3(0.0, 0.0, yaw)

    --RootMotion combo should start from the new camera yaw--
    if ctx.obj.GetController ~= nil then
        local controller = ctx.obj:GetController()
        if controller ~= nil and controller.GetControlRotation ~= nil and controller.SetControlRotation ~= nil then
            local controlRotation = controller:GetControlRotation()
            if controlRotation ~= nil then
                controller:SetControlRotation(Vec3(controlRotation.X or 0.0, controlRotation.Y or 0.0, yaw))
            else
                controller:SetControlRotation(Vec3(0.0, 0.0, yaw))
            end
        end
    end
end

function Context.NormalizeFlatDirection(direction)
    if direction == nil then
        return nil
    end

    direction.Z = 0.0
    if direction:Length() <= 0.0001 then
        return nil
    end

    return direction:Normalized()
end

return Context
