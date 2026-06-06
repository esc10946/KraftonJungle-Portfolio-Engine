local PLAYER_TAGS = { "Player", "PlayerCharacter" }
local PLAYER_CLASSES = { "ALuaCharacter", "LuaCharacter", "ACharacter", "Character" }

local ATTACK_MONTAGE_PATH = "Content/Montages/Attack1_Montage.uasset"

local ATTACK_RANGE = 300.0
local ATTACK_INTERVAL = 2.0
local WAKEUP_DELAY = 0.5

local attackMontage = nil
local playerActor = nil
local anim = nil
local nextAttackTime = 0.0

local function now()
    if World ~= nil and World.GetGameTime ~= nil then
        return World.GetGameTime()
    end

    return 0.0
end

local function is_valid(object)
    if object == nil then
        return false
    end

    if Object ~= nil and Object.IsValid ~= nil then
        return Object.IsValid(object)
    end

    if object.IsValid ~= nil then
        return object:IsValid()
    end

    return true
end

local function call_object(object, functionName, ...)
    if object == nil then
        return nil
    end

    local directFunction = object[functionName]
    if directFunction ~= nil then
        return directFunction(object, ...)
    end

    if object.CallFunction ~= nil then
        return object:CallFunction(functionName, ...)
    end

    if Object ~= nil and Object.CallFunction ~= nil then
        return Object.CallFunction(object, functionName, ...)
    end

    return nil
end

local function load_assets()
    if attackMontage ~= nil or Animation == nil or Animation.LoadMontage == nil then
        return
    end

    attackMontage = Animation.LoadMontage(ATTACK_MONTAGE_PATH)
end

local function find_player()
    if World == nil then
        return nil
    end

    local function is_player_candidate(actor)
        return is_valid(actor) and actor ~= obj
    end

    if World.FindFirstActorByTag ~= nil then
        for _, tag in ipairs(PLAYER_TAGS) do
            local actor = World.FindFirstActorByTag(tag)
            if is_player_candidate(actor) then
                return actor
            end
        end
    end

    if World.FindFirstActorByClass ~= nil then
        for _, className in ipairs(PLAYER_CLASSES) do
            local actor = World.FindFirstActorByClass(className)
            if is_player_candidate(actor) then
                return actor
            end
        end
    end

    return nil
end

local function get_actor_location(actor)
    if actor == nil then
        return nil
    end

    if actor.Location ~= nil then
        return actor.Location
    end

    return call_object(actor, "GetActorLocation")
end

local function get_anim_instance(actor)
    if not is_valid(actor) then
        return nil
    end

    local mesh = nil
    if actor.GetSkeletalMeshComponent ~= nil then
        mesh = actor:GetSkeletalMeshComponent()
    end

    if mesh == nil then
        mesh = call_object(actor, "GetSkeletalMeshComponent")
    end

    if mesh == nil and actor.GetMesh ~= nil then
        mesh = actor:GetMesh()
    end

    if mesh == nil then
        mesh = call_object(actor, "GetMesh")
    end

    if mesh == nil or mesh.GetAnimInstance == nil then
        return nil
    end

    return mesh:GetAnimInstance()
end

local function cache_refs()
    if not is_valid(playerActor) then
        playerActor = find_player()
    end

    if not is_valid(anim) then
        anim = get_anim_instance(obj)
    end

    return is_valid(playerActor) and is_valid(anim)
end

local function get_distance_to_player()
    local selfLocation = get_actor_location(obj)
    local playerLocation = get_actor_location(playerActor)
    if selfLocation == nil or playerLocation == nil then
        return 999999.0
    end

    local delta = playerLocation - selfLocation
    return delta:Length()
end

local function face_player()
    local selfLocation = get_actor_location(obj)
    local playerLocation = get_actor_location(playerActor)
    if selfLocation == nil or playerLocation == nil then
        return
    end

    local delta = playerLocation - selfLocation
    delta.Z = 0.0
    if delta:Length() <= 0.0001 then
        return
    end

    local yaw = 0.0
    if math.atan2 ~= nil then
        yaw = math.deg(math.atan2(delta.Y, delta.X))
    else
        yaw = math.deg(math.atan(delta.Y / delta.X))
        if delta.X < 0.0 then
            yaw = yaw + 180.0
        end
    end

    obj.Rotation = Vec3(0.0, 0.0, yaw)
end

local function is_ragdolled()
    return call_object(obj, "IsInRagdoll") == true
end

local function is_attacking()
    return anim ~= nil
        and attackMontage ~= nil
        and anim.IsMontagePlaying ~= nil
        and anim:IsMontagePlaying(attackMontage)
end

local function can_attack()
    return attackMontage ~= nil
        and not is_ragdolled()
        and cache_refs()
        and get_distance_to_player() <= ATTACK_RANGE
        and not is_attacking()
end

local function play_attack()
    if not can_attack() then
        return false
    end

    face_player()
    anim:PlayMontage(attackMontage)
    print("[TestParryMonster] play attack montage")
    return true
end

function BeginPlay()
    load_assets()
    cache_refs()
    nextAttackTime = now() + WAKEUP_DELAY
    print("[TestParryMonster] BeginPlay")
end

function Tick(dt)
    if is_ragdolled() then
        return
    end

    local time = now()
    if time < nextAttackTime then
        return
    end

    if play_attack() then
        nextAttackTime = time + ATTACK_INTERVAL
    else
        nextAttackTime = time + 0.2
    end
end
