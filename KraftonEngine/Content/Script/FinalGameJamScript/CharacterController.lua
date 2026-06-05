local LMB = "LeftMouseButton"
local MOVE_KEYS = { "W", "A", "S", "D" }
local WEAPON_MESH_PATH = "Content/Data/FGJ_Character/Weapon/Katana_StaticMesh.uasset"
local RWEAPON_SOCKET = "RH_Socket"
local LWEAPON_SOCKET = "LH_Socket"
local GROUND_SPEED_VAR = "GroundSpeed"

local ATTACK_MONTAGE_PATHS = {
    "Content/Montages/Attack1_Montage.uasset",
    "Content/Montages/Attack2_Montage.uasset",
    "Content/Montages/Attack3_Montage.uasset",
    "Content/Montages/Attack4_Montage.uasset",
}

local attackMontages = {} --Save Loaded Montage"
local animInstance = nil --Cached Character's SkeletalMesh animInstance
local RweaponComponent = nil --Cached equipped weapon component
local LweaponComponent = nil
local weaponBindingWarned = false --Print binding warning once
local currentAttackIndex = 0
local attackPlaying = false
local canChainAttack = false --EnableAttack Notify opens this flag
local movementLocked = false

--Get AnimInstance from SkeletalMesh--
local function GetOrCacheAnimInstance()
    if animInstance ~= nil or obj == nil or obj.GetSkeletalMeshComponent == nil then
        return animInstance
    end

    local mesh = obj:GetSkeletalMeshComponent()
    if mesh == nil then
        return nil
    end

    animInstance = mesh:GetAnimInstance()
    return animInstance
end

--Equip Weapon to RH_Socket--
local function equip_weapon()
    if RweaponComponent ~= nil or obj == nil then
        return
    end
    if LweaponComponent ~= nil or obj == nil then
        return
    end


    if Equipment == nil or Equipment.AttachStaticMeshToSocket == nil then
        if not weaponBindingWarned then
            --If this prints, rebuild C++ first--
            print("[CharacterController] Equipment.AttachStaticMeshToSocket binding missing")
            weaponBindingWarned = true
        end
        return
    end

    --Create weapon component and attach to hand socket--
    LweaponComponent = Equipment.AttachStaticMeshToSocket(obj, WEAPON_MESH_PATH, LWEAPON_SOCKET, Vec3(1.0,1.0,1.0))
    RweaponComponent = Equipment.AttachStaticMeshToSocket(obj, WEAPON_MESH_PATH, RWEAPON_SOCKET, Vec3(1.0,1.0,1.0))
end

--Load AnimMontages from Saved Loaded Montages--
local function load_attack_montages()
    if #attackMontages > 0 or Animation == nil or Animation.LoadMontage == nil then
        return
    end

    for i, path in ipairs(ATTACK_MONTAGE_PATHS) do
        attackMontages[i] = Animation.LoadMontage(path)
    end
end

--Consume EnableAttack Notify from C++--
local function consume_enable_attack(anim)
    if anim == nil or Animation == nil or Animation.ConsumeEnableAttack == nil then
        return false
    end

    return Animation.ConsumeEnableAttack(anim)
end

local function get_character_movement()
    if obj == nil then
        return nil
    end

    --obj is bound as Actor, so ACharacter functions can be hidden--
    if obj.GetCharacterMovement ~= nil then
        return obj:GetCharacterMovement()
    end

    --Call reflected ACharacter function when obj's lua type is Actor--
    if obj.IsA ~= nil and obj.CallFunction ~= nil and obj:IsA("ACharacter") then
        return obj:CallFunction("GetCharacterMovement")
    end

    return nil
end

local function call_movement_function(movement, functionName, ...)
    if movement == nil then
        return nil
    end

    local directFunction = movement[functionName]
    if directFunction ~= nil then
        return directFunction(movement, ...)
    end

    if movement.CallFunction ~= nil then
        return movement:CallFunction(functionName, ...)
    end

    return nil
end

local function set_movement_input_enabled(enabled)
    local movement = get_character_movement()
    if movement == nil then
        return
    end

    call_movement_function(movement, "SetMovementInputEnabled", enabled)
end

local function stop_horizontal_movement()
    local movement = get_character_movement()
    if movement == nil then
        return
    end

    call_movement_function(movement, "ClearInputVector")
    call_movement_function(movement, "StopHorizontalMovementImmediately")
end

local function lock_movement_for_attack()
    movementLocked = true
    set_movement_input_enabled(false)
    stop_horizontal_movement()
end

local function unlock_movement_after_attack()
    if not movementLocked then
        return
    end

    movementLocked = false
    set_movement_input_enabled(true)
end

local function is_move_key_down()
    for _, key in ipairs(MOVE_KEYS) do
        if Input.GetKey(key) then
            return true
        end
    end

    return false
end

local function update_movement_lock()
    if attackPlaying and not canChainAttack then
        lock_movement_for_attack()
        return
    end

    unlock_movement_after_attack()

    --Stop immediately when all movement keys are released--
    if not is_move_key_down() then
        stop_horizontal_movement()
    end
end

local function stop_attack_for_movement()
    if not attackPlaying or not canChainAttack or not is_move_key_down() then
        return
    end

    local anim = GetOrCacheAnimInstance()
    if anim ~= nil and anim.StopMontage ~= nil then
        --Stop montage immediately and return to locomotion pose--
        anim:StopMontage(0.0)
    end

    attackPlaying = false
    currentAttackIndex = 0
    canChainAttack = false
    unlock_movement_after_attack()
    consume_enable_attack(anim)
end

local function configure_movement()
    local movement = get_character_movement()
    if movement == nil then
        return
    end

    --Make key release stop horizontal movement immediately--
    call_movement_function(movement, "SetStopImmediatelyWhenNoInput", true)
end

--Play Attack--
local function play_attack(index)
    --If Invalid just return--
    local anim = GetOrCacheAnimInstance()
    local montage = attackMontages[index]
    if anim == nil or montage == nil then
        attackPlaying = false
        currentAttackIndex = 0
        return
    end

    --If Valid Attack--
    currentAttackIndex = index
    attackPlaying = true
    canChainAttack = false
    lock_movement_for_attack()
    --Clear old EnableAttack flag--
    consume_enable_attack(anim)
    anim:PlayMontage(montage)
end

local function is_attack_pressed()
    return Input.GetKeyDown(LMB)
end

--Update AnimGraph GroundSpeed variable--
local function get_movement_velocity(movement)
    if movement == nil then
        return nil
    end

    --Use direct binding when movement is bound as CharacterMovementComponent--
    if movement.GetVelocity ~= nil then
        return movement:GetVelocity()
    end

    --Use reflected function when movement is bound as Object/Component--
    if movement.CallFunction ~= nil then
        return movement:CallFunction("GetVelocityValue")
    end

    return nil
end

local function update_ground_speed()
    local groundSpeed = 0.0

    local movement = get_character_movement()
    if movement ~= nil then
        local velocity = get_movement_velocity(movement)
        if velocity ~= nil then
            groundSpeed = math.sqrt(velocity.X * velocity.X + velocity.Y * velocity.Y)
        end
    end

    local anim = GetOrCacheAnimInstance()
    if anim ~= nil and anim.SetGraphVariableFloat ~= nil then
        anim:SetGraphVariableFloat(GROUND_SPEED_VAR, groundSpeed)
    end

end

--Consume EnableAttack Notify flag--
local function update_attack_chain_window()
    if not attackPlaying then
        return
    end

    local anim = GetOrCacheAnimInstance()
    if consume_enable_attack(anim) then
        canChainAttack = true
    end
end

--Play next attack if EnableAttack Notify opened chain--
local function try_play_next_attack()
    if not attackPlaying or not canChainAttack then
        return false
    end

    local nextIndex = currentAttackIndex + 1
    if nextIndex <= #ATTACK_MONTAGE_PATHS then
        play_attack(nextIndex)
        return true
    end

    return false
end

local function start_attack_sequence()
    --fill in attackMontages--
    load_attack_montages()
    --start attack--
    play_attack(1)
end

--Handle first attack and combo input--
local function handle_attack_input()
    if not is_attack_pressed() then
        return
    end

    if attackPlaying then
        try_play_next_attack()
        return
    end

    start_attack_sequence()
end

--End attack sequence when current montage is finished--
local function update_attack_sequence()
    if not attackPlaying then
        return
    end

    local anim = GetOrCacheAnimInstance()
    local montage = attackMontages[currentAttackIndex]
    if anim == nil or montage == nil then
        attackPlaying = false
        currentAttackIndex = 0
        canChainAttack = false
        unlock_movement_after_attack()
        return
    end

    if anim:IsMontagePlaying(montage) then
        return
    end

    attackPlaying = false
    currentAttackIndex = 0
    canChainAttack = false
    unlock_movement_after_attack()
    consume_enable_attack(anim)
end

function BeginPlay()
    GetOrCacheAnimInstance()
    configure_movement()
    equip_weapon()
    load_attack_montages()
end

function Tick(dt)
    update_attack_chain_window()
    update_movement_lock()
    stop_attack_for_movement()
    update_ground_speed()
    handle_attack_input()
    update_attack_sequence()
end
