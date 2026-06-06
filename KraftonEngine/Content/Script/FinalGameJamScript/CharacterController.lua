local LMB = "LeftMouseButton"
local RMB = "RightMouseButton"
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

local DEFENSE_IDLE_MONTAGE_PATH = "Content/Montages/DefenseIdle_Montage.uasset"
local SUCCESS_PARRY_MONTAGE_PATH = "Content/Montages/SuccessParry_Montage.uasset"
local ATTACK_PLAY_RATE = 1.8
local HIT_MONTAGE_PATHS = {
    F = "Content/Montages/Hit_F_Montage.uasset",
    B = "Content/Montages/Hit_B_Montage.uasset",
    L = "Content/Montages/Hit_L_Montage.uasset",
    R = "Content/Montages/Hit_R_Montage.uasset",
}

local attackMontages = {} --Save Loaded Montage"
local hitMontages = {}
local defenseIdleMontage = nil
local successParryMontage = nil
local animInstance = nil --Cached Character's SkeletalMesh animInstance
local RweaponComponent = nil --Cached equipped weapon component
local LweaponComponent = nil
local weaponBindingWarned = false --Print binding warning once
local currentAttackIndex = 0
local attackPlaying = false
local canChainAttack = false --EnableAttack Notify opens this flag
local attackInputQueued = false --Save attack input until EnableAttack opens
local defensePlaying = false
local defenseIdlePlaying = false
local successParryPlaying = false
local hitPlaying = false
local currentHitMontage = nil
local canCancelHit = false --EnableAttack Notify opens hit cancel
local playerDefeated = false --Prevent duplicate game end
local parryWindowOpen = false
local movementLocked = false
local commandList = {}

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

--Load Parry Montages--
local function load_parry_montages()
    if Animation == nil or Animation.LoadMontage == nil then
        return
    end

    if defenseIdleMontage == nil then
        defenseIdleMontage = Animation.LoadMontage(DEFENSE_IDLE_MONTAGE_PATH)
    end

    if successParryMontage == nil then
        successParryMontage = Animation.LoadMontage(SUCCESS_PARRY_MONTAGE_PATH)
    end
end

--Load Hit Montages--
local function load_hit_montages()
    if Animation == nil or Animation.LoadMontage == nil then
        return
    end

    for key, path in pairs(HIT_MONTAGE_PATHS) do
        if hitMontages[key] == nil then
            hitMontages[key] = Animation.LoadMontage(path)
        end
    end
end

--Consume EnableAttack Notify from C++--
local function consume_enable_attack(anim)
    if anim == nil or Animation == nil or Animation.ConsumeEnableAttack == nil then
        return false
    end

    return Animation.ConsumeEnableAttack(anim)
end

--Check ParryWindow NotifyState from C++--
local function is_parry_window_active(anim)
    if anim == nil or Animation == nil or Animation.IsParryWindowActive == nil then
        return false
    end

    return Animation.IsParryWindowActive(anim)
end

--Consume successful parry event from C++--
local function consume_successful_parry(anim)
    if anim == nil or Animation == nil or Animation.ConsumeSuccessfulParry == nil then
        return false
    end

    return Animation.ConsumeSuccessfulParry(anim)
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
    if attackPlaying or defensePlaying or successParryPlaying or (hitPlaying and not canCancelHit) then
        lock_movement_for_attack()
        return
    end

    unlock_movement_after_attack()

    --Stop immediately when all movement keys are released--
    if not is_move_key_down() then
        stop_horizontal_movement()
    end
end

local function reset_attack_state()
    attackPlaying = false
    currentAttackIndex = 0
    canChainAttack = false
    attackInputQueued = false
end

local function reset_parry_state()
    defensePlaying = false
    defenseIdlePlaying = false
    successParryPlaying = false
    parryWindowOpen = false
end

local function reset_hit_state()
    hitPlaying = false
    currentHitMontage = nil
    canCancelHit = false
end

local function should_end_game_from_damage_report(damageReport)
    if damageReport == nil then
        return false
    end

    if damageReport.bKilled == true then
        return true
    end

    return damageReport.NewHealth ~= nil and damageReport.NewHealth <= 0.0
end

local function stop_current_montage()
    local anim = GetOrCacheAnimInstance()
    if anim ~= nil and anim.StopMontage ~= nil then
        --Stop montage immediately--
        anim:StopMontage(0.0)
    end
end

local function request_player_defeated()
    if playerDefeated then
        return
    end

    playerDefeated = true
    stop_current_montage()
    reset_attack_state()
    reset_parry_state()
    reset_hit_state()
    lock_movement_for_attack()

    if Game ~= nil and Game.Defeated ~= nil then
        Game.Defeated()
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
    attackInputQueued = false
    unlock_movement_after_attack()
    consume_enable_attack(anim)
end

local function stop_defense()
    if not defensePlaying then
        return
    end

    stop_current_montage()
    defensePlaying = false
    defenseIdlePlaying = false
    parryWindowOpen = false
    unlock_movement_after_attack()
end

local function stop_hit_for_cancel()
    if not hitPlaying or not canCancelHit then
        return
    end

    stop_current_montage()
    reset_hit_state()
    unlock_movement_after_attack()
end

local function configure_movement()
    local movement = get_character_movement()
    if movement == nil then
        return
    end

    --Make key release stop horizontal movement immediately--
    call_movement_function(movement, "SetStopImmediatelyWhenNoInput", true)
end

local function get_active_camera()
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

local function get_camera_flat_forward()
    local camera = get_active_camera()
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

local function face_camera_forward()
    if obj == nil then
        return
    end

    local forward = get_camera_flat_forward()
    if forward == nil then
        return
    end

    local yaw = atan2_degrees(forward.Y, forward.X)
    --FVector rotation uses X=Roll, Y=Pitch, Z=Yaw--
    obj.Rotation = Vec3(0.0, 0.0, yaw)
end

local function normalize_flat_direction(direction)
    if direction == nil then
        return nil
    end

    direction.Z = 0.0
    if direction:Length() <= 0.0001 then
        return nil
    end

    return direction:Normalized()
end

local function get_damage_source_direction(damageSpec)
    if damageSpec == nil or obj == nil or obj.Location == nil then
        return nil
    end

    --Use attacker position first--
    if damageSpec.DamageCauser ~= nil and damageSpec.DamageCauser.Location ~= nil then
        local sourceDirection = damageSpec.DamageCauser.Location - obj.Location
        local flat = normalize_flat_direction(sourceDirection)
        if flat ~= nil then
            return flat
        end
    end

    --Fallback to instigator position--
    if damageSpec.InstigatorActor ~= nil and damageSpec.InstigatorActor.Location ~= nil then
        local sourceDirection = damageSpec.InstigatorActor.Location - obj.Location
        return normalize_flat_direction(sourceDirection)
    end

    --Fallback to attack travel direction--
    if damageSpec.HitDirection ~= nil then
        local sourceDirection = damageSpec.HitDirection * -1.0
        return normalize_flat_direction(sourceDirection)
    end

    return nil
end

local function choose_hit_direction_key(damageSpec)
    local sourceDirection = get_damage_source_direction(damageSpec)
    if sourceDirection == nil or obj == nil or obj.Forward == nil or obj.Right == nil then
        return "F"
    end

    local forward = normalize_flat_direction(obj.Forward)
    local right = normalize_flat_direction(obj.Right)
    if forward == nil or right == nil then
        return "F"
    end

    local forwardDot = sourceDirection:Dot(forward)
    local rightDot = sourceDirection:Dot(right)
    if math.abs(forwardDot) >= math.abs(rightDot) then
        if forwardDot >= 0.0 then
            return "F"
        end
        return "B"
    end

    if rightDot >= 0.0 then
        return "L"
    end
    return "R"
end

--Play Hit Reaction--
local function play_hit_reaction(damageSpec)
    load_hit_montages()

    local anim = GetOrCacheAnimInstance()
    if anim == nil then
        return
    end

    local key = choose_hit_direction_key(damageSpec)
    local montage = hitMontages[key]
    if montage == nil then
        return
    end

    stop_current_montage()
    reset_attack_state()
    reset_parry_state()
    hitPlaying = true
    currentHitMontage = montage
    canCancelHit = false
    lock_movement_for_attack()
    --Clear old EnableAttack flag--
    consume_enable_attack(anim)
    anim:PlayMontage(montage, nil, ATTACK_PLAY_RATE)
end

--Play Attack--
local function play_attack(index)
    --If Invalid just return--
    local anim = GetOrCacheAnimInstance()
    local montage = attackMontages[index]
    if anim == nil or montage == nil then
        attackPlaying = false
        currentAttackIndex = 0
        canChainAttack = false
        attackInputQueued = false
        return
    end

    --If Valid Attack--
    currentAttackIndex = index
    attackPlaying = true
    canChainAttack = false
    attackInputQueued = false
    face_camera_forward()
    lock_movement_for_attack()
    --Clear old EnableAttack flag--
    consume_enable_attack(anim)
    anim:PlayMontage(montage)
end

local function is_attack_pressed()
    return Input.GetKeyDown(LMB)
end

local function is_parry_pressed()
    return Input.GetKeyDown(RMB)
end

local function is_parry_down()
    return Input.GetKey(RMB)
end

local function is_parry_released()
    return Input.GetKeyUp(RMB)
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

--Consume Hit cancel Notify flag--
local function update_hit_cancel_window()
    if not hitPlaying or canCancelHit then
        return
    end

    local anim = GetOrCacheAnimInstance()
    if consume_enable_attack(anim) then
        canCancelHit = true
        unlock_movement_after_attack()
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

local function try_play_queued_attack()
    if not attackInputQueued or not attackPlaying or not canChainAttack then
        return false
    end

    --Use buffered attack before movement cancel--
    attackInputQueued = false
    return try_play_next_attack()
end

local function start_attack_sequence()
    --fill in attackMontages--
    load_attack_montages()
    --start attack--
    play_attack(1)
end

local function play_defense_idle()
    if defenseIdlePlaying then
        return
    end

    load_parry_montages()

    local anim = GetOrCacheAnimInstance()
    if anim == nil or defenseIdleMontage == nil then
        reset_parry_state()
        return
    end

    stop_current_montage()
    reset_attack_state()
    face_camera_forward()

    defensePlaying = true
    defenseIdlePlaying = true
    successParryPlaying = false
    parryWindowOpen = false

    lock_movement_for_attack()
    anim:PlayMontage(defenseIdleMontage)
end

local function play_success_parry()
    local anim = GetOrCacheAnimInstance()
    if anim == nil or successParryMontage == nil then
        reset_parry_state()
        unlock_movement_after_attack()
        return
    end

    stop_current_montage()
    defensePlaying = false
    defenseIdlePlaying = false
    successParryPlaying = true
    parryWindowOpen = false
    lock_movement_for_attack()
    anim:PlayMontage(successParryMontage)
end

local function execute_attack_input()
    if defensePlaying or successParryPlaying then
        return
    end

    if hitPlaying then
        if not canCancelHit then
            return
        end

        stop_hit_for_cancel()
    end

    if attackPlaying then
        if not canChainAttack then
            --Queue combo input until EnableAttack Notify opens--
            attackInputQueued = true
            return
        end

        try_play_next_attack()
        return
    end

    start_attack_sequence()
end

local function execute_parry_input()
    if attackPlaying or defensePlaying or successParryPlaying then
        return
    end

    if hitPlaying then
        if not canCancelHit then
            return
        end

        --EnableAttack Notify opens guard cancel from hit--
        stop_hit_for_cancel()
    end

    if playerDefeated then
        return
    end

    play_defense_idle()
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
        attackInputQueued = false
        unlock_movement_after_attack()
        return
    end

    if anim:IsMontagePlaying(montage) then
        return
    end

    attackPlaying = false
    currentAttackIndex = 0
    canChainAttack = false
    attackInputQueued = false
    unlock_movement_after_attack()
    consume_enable_attack(anim)
end

local function update_parry_window()
    if not defensePlaying then
        parryWindowOpen = false
        return
    end

    parryWindowOpen = is_parry_window_active(GetOrCacheAnimInstance())
end

local function update_success_parry()
    if not defensePlaying then
        return
    end

    local anim = GetOrCacheAnimInstance()
    if consume_successful_parry(anim) then
        play_success_parry()
    end
end

local function update_defense_sequence()
    if not defensePlaying then
        return
    end

    if is_parry_released() then
        stop_defense()
        return
    end

    local anim = GetOrCacheAnimInstance()
    if anim == nil then
        reset_parry_state()
        unlock_movement_after_attack()
        return
    end

    if defenseIdlePlaying and defenseIdleMontage ~= nil and not anim:IsMontagePlaying(defenseIdleMontage) then
        if is_parry_down() then
            defenseIdlePlaying = false
            play_defense_idle()
            return
        end

        stop_defense()
        return
    end

end

local function update_success_parry_sequence()
    if not successParryPlaying then
        return
    end

    local anim = GetOrCacheAnimInstance()
    if anim == nil or successParryMontage == nil or not anim:IsMontagePlaying(successParryMontage) then
        successParryPlaying = false
        unlock_movement_after_attack()
    end
end

local function update_hit_sequence()
    if not hitPlaying then
        return
    end

    if canCancelHit and is_move_key_down() then
        stop_hit_for_cancel()
        return
    end

    local anim = GetOrCacheAnimInstance()
    if anim == nil or currentHitMontage == nil or not anim:IsMontagePlaying(currentHitMontage) then
        reset_hit_state()
        unlock_movement_after_attack()
    end
end

--Register command in Tick order--
local function register_command(command)
    commandList[#commandList + 1] = command
end

--Run one command--
local function run_command(command, dt)
    if command.Tick ~= nil then
        command:Tick(dt)
    end

    if command.Execute == nil then
        return
    end

    if command.IsTriggered ~= nil and not command:IsTriggered() then
        return
    end

    if command.CanExecute ~= nil and not command:CanExecute() then
        return
    end

    command:Execute(dt)
end

--Run registered commands--
local function run_commands(dt)
    for _, command in ipairs(commandList) do
        run_command(command, dt)
    end
end

local AttackWindowCommand = {}

function AttackWindowCommand:Tick(dt)
    update_attack_chain_window()
end

local HitCancelWindowCommand = {}

function HitCancelWindowCommand:Tick(dt)
    update_hit_cancel_window()
end

local ParryWindowCommand = {}

function ParryWindowCommand:Tick(dt)
    update_parry_window()
end

local ParrySuccessCommand = {}

function ParrySuccessCommand:Tick(dt)
    update_success_parry()
end

local MovementLockCommand = {}

function MovementLockCommand:Tick(dt)
    update_movement_lock()
end

local MoveCancelAttackCommand = {}

function MoveCancelAttackCommand:IsTriggered()
    return is_move_key_down()
end

function MoveCancelAttackCommand:CanExecute()
    return attackPlaying and canChainAttack
end

function MoveCancelAttackCommand:Execute(dt)
    stop_attack_for_movement()
end

local GroundSpeedCommand = {}

function GroundSpeedCommand:Tick(dt)
    update_ground_speed()
end

local AttackCommand = {}

function AttackCommand:IsTriggered()
    return is_attack_pressed()
end

function AttackCommand:Execute(dt)
    execute_attack_input()
end

local ParryCommand = {}

function ParryCommand:IsTriggered()
    return is_parry_pressed()
end

function ParryCommand:Execute(dt)
    execute_parry_input()
end

local QueuedAttackCommand = {}

function QueuedAttackCommand:CanExecute()
    return attackInputQueued and attackPlaying and canChainAttack
end

function QueuedAttackCommand:Execute(dt)
    try_play_queued_attack()
end

local AttackLifecycleCommand = {}

function AttackLifecycleCommand:Tick(dt)
    update_attack_sequence()
end

local ParryLifecycleCommand = {}

function ParryLifecycleCommand:Tick(dt)
    update_defense_sequence()
    update_success_parry_sequence()
end

local HitLifecycleCommand = {}

function HitLifecycleCommand:Tick(dt)
    update_hit_sequence()
end

--Setup player commands--
local function setup_commands()
    if #commandList > 0 then
        return
    end

    register_command(AttackWindowCommand)
    register_command(HitCancelWindowCommand)
    register_command(ParryWindowCommand)
    register_command(ParrySuccessCommand)
    register_command(MovementLockCommand)
    register_command(ParryCommand)
    --Attack input comes before movement cancel to keep combo while holding move key--
    register_command(AttackCommand)
    register_command(QueuedAttackCommand)
    register_command(MoveCancelAttackCommand)
    register_command(GroundSpeedCommand)
    register_command(AttackLifecycleCommand)
    register_command(ParryLifecycleCommand)
    register_command(HitLifecycleCommand)
end

function BeginPlay()
    GetOrCacheAnimInstance()
    configure_movement()
    equip_weapon()
    load_attack_montages()
    load_parry_montages()
    load_hit_montages()
    setup_commands()
end

function Tick(dt)
    if playerDefeated then
        return
    end

    run_commands(dt)
end

function OnDamaged(damageSpec, damageReport)
    if damageReport ~= nil and damageReport.AppliedDamage ~= nil and damageReport.AppliedDamage <= 0.0 then
        return
    end

    if should_end_game_from_damage_report(damageReport) then
        request_player_defeated()
        return
    end

    play_hit_reaction(damageSpec)
end
