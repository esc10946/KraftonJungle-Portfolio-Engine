local LMB = "LeftMouseButton"
local WEAPON_MESH_PATH = "Content/Data/FGJ_Character/Weapon/Katana_StaticMesh.uasset"
local WEAPON_SOCKET = "RH_Socket"
local WEAPON_SCALE = Vec3(1.0,1.0,1.0)--Vec3(0.053, 0.396, 0.082)

local ATTACK_MONTAGE_PATHS = {
    "Content/Montages/HorizontalAttack_Unreal_Take_Montage.uasset",
    "Content/Montages/VerticalAttack_Unreal_Take_Montage.uasset",
    "Content/Montages/360Attack_Unreal_Take_Montage.uasset",
}

local attackMontages = {} --Save Loaded Montage"
local animInstance = nil --Cached Character's SkeletalMesh animInstance
local weaponComponent = nil --Cached equipped weapon component
local weaponBindingWarned = false --Print binding warning once
local currentAttackIndex = 0
local attackPlaying = false
local canChainAttack = false --EnableAttack Notify opens this flag

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
    if weaponComponent ~= nil or obj == nil then
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
    weaponComponent = Equipment.AttachStaticMeshToSocket(obj, WEAPON_MESH_PATH, WEAPON_SOCKET, WEAPON_SCALE)
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
    --Clear old EnableAttack flag--
    consume_enable_attack(anim)
    anim:PlayMontage(montage)
end

local function is_attack_pressed()
    return Input.GetKeyDown(LMB)
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
        return
    end

    if anim:IsMontagePlaying(montage) then
        return
    end

    attackPlaying = false
    currentAttackIndex = 0
    canChainAttack = false
    consume_enable_attack(anim)
end

function BeginPlay()
    GetOrCacheAnimInstance()
    equip_weapon()
    load_attack_montages()
end

function Tick(dt)
    update_attack_chain_window()
    handle_attack_input()
    update_attack_sequence()
end
