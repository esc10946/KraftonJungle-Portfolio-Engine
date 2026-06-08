-- Generated runtime for Content/Blueprint/BossIntroDirectorBP.uasset.
-- Dormant boss flow:
--   BloodMoonPhase -> StartIntro() -> reveal boss intro -> StartBossEncounter()
--   -> watch boss death -> beat -> Game.OnBossSlain() (-> Victory phase).
__events = { BeginPlay = true, PostBeginPlay = false, Tick = true, PostStartMatch = true, OnPlayerCameraReady = false, EndPlay = true, OnOverlap = false, OnEndOverlap = false, OnHit = false, OnEndHit = false }

local INTRO_TIME        = 3.0
local VICTORY_BEAT      = 1.5
local STOP_SHORT        = 13.0
local MAX_WALK_DISTANCE = 1.2
local KATANA    = "Content/Data/FGJ_Character/Weapon/Katana_StaticMesh.uasset"
local BOSS_WALK = "Content/Data/Samura2/source/Standing Walk Forward_mixamo_com.uasset"
local BOSS_IDLE = "Content/Data/Samura2/source/Standing Idle_mixamo_com.uasset"
local HAND_BONES = { "hand_r", "RightHand", "mixamorig:RightHand", "weapon_r" }
local HUD_HIDE_FLAG = "BossIntroHUDHidden"
local START_FLAG = "BossIntroDirectorStartOk"
local BOSS_TAG = "Boss"
local BLOOD_MOON_TAG = "BloodMoon"

local COLLISION_NONE = 0
local COLLISION_QUERY_AND_PHYSICS = 3

local boss, player, cine, bloodMoon
local t = 0.0
local started = false
local setup_done = false
local handed_off = false
local hud_restored_after_intro = false
local boss_dormant = false
local intro_cutscene_entered = false
local intro_cinematic_fired = false
local walk_from, walk_to
local collision_state = {}
local boss_slain = false
local victory_fired = false
local death_elapsed = 0.0

local function dbg(m) if print then print("[BossIntroDirector] " .. tostring(m)) end end
local function first_by_tag(tag)
    if World and World.FindActorsByTag then local l = World.FindActorsByTag(tag); if l and #l > 0 then return l[1] end end
    return nil
end
local function first_by_class(cn)
    if World and World.FindActorsByClass then local l = World.FindActorsByClass(cn); if l and #l > 0 then return l[1] end end
    return nil
end
local function rcall(o, fn, ...) if o and Reflection and Reflection.Call then return Reflection.Call(o, fn, ...) end return nil end
local function call(o, fn, ...)
    if not o then return nil end
    local f = o[fn]
    if f ~= nil then
        local ok, result = pcall(f, o, ...)
        if ok then return result end
        return nil
    end
    return rcall(o, fn, ...)
end
local function lerp(a,b,s) return a + (b-a)*s end
local function is_valid(o)
    if o == nil then return false end
    if IsValid then return IsValid(o) end
    return true
end
local function set_hud_visible(visible)
    if _G ~= nil then
        _G[HUD_HIDE_FLAG] = visible ~= true
    end

    local hud = HUD or (_G and _G.HUD)
    if hud and hud.SetHUDVisible then
        hud.SetHUDVisible(visible)
        return true
    end

    return false
end
local function hide_hud_for_intro()
    if hud_restored_after_intro then return end
    set_hud_visible(false)
end
local function restore_hud_after_intro()
    if hud_restored_after_intro then return end
    set_hud_visible(true)
    hud_restored_after_intro = true
end
local function show_boss_ui_after_intro()
    local hud = HUD or (_G and _G.HUD)

    if not hud then
        return false
    end

    if boss and hud.BindBossActor then
        local ok, result = pcall(function() return hud.BindBossActor(boss, true) end)
        if ok then
            return result == true
        end
    end

    if hud.SetBossHUDVisible then
        hud.SetBossHUDVisible(true)
        return true
    end

    if hud.SetBossVisibility then
        hud.SetBossVisibility(true)
        return true
    end

    return false
end
local function hide_boss_ui()
    local hud = HUD or (_G and _G.HUD)
    if hud and hud.ClearBossActor then
        hud.ClearBossActor()
    elseif hud and hud.SetBossHUDVisible then
        hud.SetBossHUDVisible(false)
    elseif hud and hud.SetBossVisibility then
        hud.SetBossVisibility(false)
    end
end
local function ensure_player_possessed()
    pcall(function()
        player = player or first_by_tag("Player") or first_by_class("ALuaCharacter")
        if World and World.GetFirstPlayerController and player then
            local pc = World.GetFirstPlayerController()
            if pc and pc.GetPossessedPawn and pc:GetPossessedPawn() ~= player then pc:Possess(player) end
        end
    end)
end
local function resolve_boss()
    boss = boss or first_by_tag(BOSS_TAG) or first_by_class("ABossEnemyCharacter") or first_by_class("BossEnemyCharacter")
    return boss
end
local function resolve_blood_moon()
    bloodMoon = bloodMoon or first_by_tag(BLOOD_MOON_TAG)
    return bloodMoon
end
local function set_blood_moon_visible(visible)
    if Scene and Scene.SetActorVisible and resolve_blood_moon() then
        Scene.SetActorVisible(bloodMoon, visible)
    end
end
local function for_each_primitive(actor, fn)
    if not actor or not actor.GetComponents then return end
    local seen = {}
    local function visit(component)
        if not component then return end
        local prim = component
        if component.AsPrimitiveComponent then
            prim = component:AsPrimitiveComponent()
        end
        if prim and not seen[prim] then
            seen[prim] = true
            fn(prim)
        end
    end

    local components = actor:GetComponents()
    if components then
        for _, component in ipairs(components) do visit(component) end
    end
    visit(call(actor, "GetCapsuleComponent"))
    visit(call(actor, "GetSkeletalMeshComponent"))
    visit(call(actor, "GetPrimitiveComponent"))
end
local function set_primitive_collision(primitive, mode)
    if not primitive then return end
    if primitive.SetCollisionEnabled then
        primitive:SetCollisionEnabled(mode)
    elseif PrimitiveComponent and PrimitiveComponent.SetCollisionEnabled then
        PrimitiveComponent.SetCollisionEnabled(primitive, mode)
    else
        rcall(primitive, "SetCollisionEnabled", mode)
    end
end
local function set_primitive_overlap(primitive, enabled)
    if not primitive then return end
    if primitive.SetGenerateOverlapEvents then
        primitive:SetGenerateOverlapEvents(enabled)
    elseif PrimitiveComponent and PrimitiveComponent.SetGenerateOverlapEvents then
        PrimitiveComponent.SetGenerateOverlapEvents(primitive, enabled)
    else
        rcall(primitive, "SetGenerateOverlapEvents", enabled)
    end
end
local function get_primitive_collision(primitive)
    if primitive and primitive.GetCollisionEnabled then
        return primitive:GetCollisionEnabled()
    elseif PrimitiveComponent and PrimitiveComponent.GetCollisionEnabled then
        return PrimitiveComponent.GetCollisionEnabled(primitive)
    end
    return rcall(primitive, "GetCollisionEnabled") or COLLISION_QUERY_AND_PHYSICS
end
local function get_primitive_overlap(primitive)
    if primitive and primitive.GetGenerateOverlapEvents then
        return primitive:GetGenerateOverlapEvents()
    end
    return rcall(primitive, "GetGenerateOverlapEvents") == true
end
local function remember_collision(actor)
    for_each_primitive(actor, function(primitive)
        if collision_state[primitive] == nil then
            collision_state[primitive] = {
                mode = get_primitive_collision(primitive),
                overlap = get_primitive_overlap(primitive),
            }
        end
    end)
end
local function set_boss_visible(visible)
    if Scene and Scene.SetActorVisible and boss then
        Scene.SetActorVisible(boss, visible)
    elseif boss then
        rcall(boss, "SetVisible", visible)
    end
end
local function set_boss_collision_enabled(enabled)
    if not boss then return end
    remember_collision(boss)
    for_each_primitive(boss, function(primitive)
        if enabled then
            local state = collision_state[primitive]
            set_primitive_collision(primitive, state and state.mode or COLLISION_QUERY_AND_PHYSICS)
            set_primitive_overlap(primitive, state and state.overlap == true)
        else
            set_primitive_overlap(primitive, false)
            set_primitive_collision(primitive, COLLISION_NONE)
        end
    end)
end
local function make_boss_dormant()
    if started or handed_off then return false end
    set_blood_moon_visible(false)
    if not resolve_boss() then return false end
    set_boss_visible(false)
    set_boss_collision_enabled(false)
    call(boss, "StopEnemyMovement")
    hide_boss_ui()
    boss_dormant = true
    return true
end
local function wake_boss_for_intro()
    if not resolve_boss() then return false end
    set_boss_visible(true)
    set_boss_collision_enabled(false)
    call(boss, "StopEnemyMovement")
    boss_dormant = false
    return true
end
local function activate_boss_encounter()
    if not boss then return false end
    set_boss_collision_enabled(true)
    if Combat and Combat.SetTeam then Combat.SetTeam(boss, 2) end
    return call(boss, "StartBossEncounter") == true
end
local function boss_is_dead()
    local health = call(boss, "GetHealthComponent")
    if not is_valid(health) then return false end
    return call(health, "IsDead") == true
end
-- Post-encounter: once the boss falls, hold a beat then resolve to Victory.
local function watch_for_victory(dt)
    if victory_fired then return end
    if not boss_slain then
        if boss_is_dead() then boss_slain = true; death_elapsed = 0.0 end
        return
    end
    death_elapsed = death_elapsed + (dt or 0.0)
    if death_elapsed >= VICTORY_BEAT then
        victory_fired = true
        if Game and Game.OnBossSlain then Game.OnBossSlain() end
        dbg("boss slain -> victory")
    end
end

function BeginPlay()
    ensure_player_possessed()
    make_boss_dormant()
end
function PostStartMatch()
    ensure_player_possessed()
    make_boss_dormant()
end
function StartIntro()
    if handed_off then
        if _G ~= nil then _G[START_FLAG] = false end
        return false
    end
    if started then
        if _G ~= nil then _G[START_FLAG] = true end
        return true
    end
    if not wake_boss_for_intro() then
        if _G ~= nil then _G[START_FLAG] = false end
        dbg("StartIntro failed: boss not found")
        return false
    end

    if _G ~= nil then _G[START_FLAG] = true end
    started = true
    setup_done = false
    hud_restored_after_intro = false
    intro_cutscene_entered = false
    intro_cinematic_fired = false
    t = 0.0
    walk_from, walk_to = nil, nil
    hide_hud_for_intro()
    ensure_player_possessed()
    dbg("intro started")
    return true
end
local function fire_intro_cinematic()
    if intro_cinematic_fired then return end
    intro_cinematic_fired = true

    set_blood_moon_visible(true)
    if Game and Game.EnterCutscene then
        Game.EnterCutscene()
        intro_cutscene_entered = true
    end
    if cine then rcall(cine,"Play") end
end
local function do_setup()
    setup_done = true
    player = player or first_by_tag("Player") or first_by_class("ALuaCharacter")
    if Combat and Combat.SetTeam and boss then Combat.SetTeam(boss, 2) end
    local attached = false
    if Equipment and Equipment.AttachStaticMeshToBone and boss then
        for _, bone in ipairs(HAND_BONES) do
            local w = Equipment.AttachStaticMeshToBone(boss, KATANA, bone, Vec3(1.0,1.0,1.0))
            if w ~= nil then attached = true; break end
        end
    end
    if Animation and Animation.PlayOnActor and boss then Animation.PlayOnActor(boss, BOSS_WALK, true) end
    pcall(function()
        local p = boss.Location
        walk_from = Vec3(p.X,p.Y,p.Z); walk_to = Vec3(p.X,p.Y,p.Z)
        if player then
            local pp = player.Location
            local dx,dy = pp.X-p.X, pp.Y-p.Y
            local d = math.sqrt(dx*dx+dy*dy)
            if d > 0.001 then
                local move = math.min(math.max(0.0, d-STOP_SHORT), MAX_WALK_DISTANCE)
                local k = move / d
                walk_to = Vec3(p.X + dx*k, p.Y + dy*k, p.Z)
            end
        end
    end)
    cine = first_by_tag("CinematicIntro") or first_by_class("ACameraCinematicActor")
    fire_intro_cinematic()
    dbg("setup: player=" .. tostring(player ~= nil) .. " boss=" .. tostring(boss ~= nil) .. " katana=" .. tostring(attached))
end
function Tick(dt)
    if not started then return end
    if handed_off then watch_for_victory(dt); return end
    hide_hud_for_intro()
    ensure_player_possessed()
    if not boss then resolve_boss() end
    if boss and not setup_done then do_setup() end
    t = t + (dt or 0.0)
    if boss and walk_from and walk_to and t < INTRO_TIME then
        pcall(function()
            local s = math.min(1.0, t / INTRO_TIME)
            boss.Location = Vec3(lerp(walk_from.X,walk_to.X,s), lerp(walk_from.Y,walk_to.Y,s), lerp(walk_from.Z,walk_to.Z,s))
        end)
    end
    if t >= INTRO_TIME then
        handed_off = true
        restore_hud_after_intro()
        if cine then rcall(cine,"Stop") end
        if boss then
            if Animation and Animation.PlayOnActor then Animation.PlayOnActor(boss, BOSS_IDLE, true) end
            activate_boss_encounter()
        end
        show_boss_ui_after_intro()
        if Game and Game.ExitCutscene then Game.ExitCutscene() end
        intro_cutscene_entered = false
        dbg("intro complete -> boss encounter active")
    end
end

function EndPlay()
    if started then restore_hud_after_intro() end
    if intro_cutscene_entered and Game and Game.ExitCutscene then Game.ExitCutscene() end
    if not handed_off and boss_dormant then hide_boss_ui() end
end
