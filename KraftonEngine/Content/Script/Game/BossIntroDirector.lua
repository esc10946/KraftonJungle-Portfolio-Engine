-- Generated runtime for Content/Blueprint/BossIntroDirectorBP.uasset.
-- BP-only intro director. It never disables player input; previous input lock came from Game.EnterCutscene.
__events = { BeginPlay = true, PostBeginPlay = false, Tick = true, PostStartMatch = true, OnPlayerCameraReady = false, EndPlay = true, OnOverlap = false, OnEndOverlap = false, OnHit = false, OnEndHit = false }

local INTRO_TIME        = 3.0
local STOP_SHORT        = 13.0
local MAX_WALK_DISTANCE = 1.2
local KATANA    = "Content/Data/FGJ_Character/Weapon/Katana_StaticMesh.uasset"
local BOSS_WALK = "Content/Data/Samura2/source/Standing Walk Forward_mixamo_com.uasset"
local BOSS_IDLE = "Content/Data/Samura2/source/Standing Idle_mixamo_com.uasset"
local HAND_BONES = { "hand_r", "RightHand", "mixamorig:RightHand", "weapon_r" }
local HUD_HIDE_FLAG = "BossIntroHUDHidden"

local boss, player, cine
local t = 0.0
local started = false
local setup_done = false
local handed_off = false
local hud_restored_after_intro = false
local walk_from, walk_to

local function dbg(m) if print then print("[BossIntroLegacy] " .. tostring(m)) end end
local function first_by_tag(tag)
    if World and World.FindActorsByTag then local l = World.FindActorsByTag(tag); if l and #l > 0 then return l[1] end end
    return nil
end
local function first_by_class(cn)
    if World and World.FindActorsByClass then local l = World.FindActorsByClass(cn); if l and #l > 0 then return l[1] end end
    return nil
end
local function rcall(o, fn, ...) if o and Reflection and Reflection.Call then return Reflection.Call(o, fn, ...) end return nil end
local function lerp(a,b,s) return a + (b-a)*s end
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
local function ensure_player_possessed()
    pcall(function()
        player = player or first_by_tag("Player") or first_by_class("ALuaCharacter")
        if World and World.GetFirstPlayerController and player then
            local pc = World.GetFirstPlayerController()
            if pc and pc.GetPossessedPawn and pc:GetPossessedPawn() ~= player then pc:Possess(player) end
        end
    end)
end
function BeginPlay()
    started = true
    hide_hud_for_intro()
    ensure_player_possessed()
    -- Ensure stale cutscene locks from earlier builds are cleared if this scene was hot-reloaded.
    if Game and Game.ExitCutscene then Game.ExitCutscene() end
end
function PostStartMatch()
    started = true
    hide_hud_for_intro()
    ensure_player_possessed()
    if Game and Game.ExitCutscene then Game.ExitCutscene() end
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
    if cine then rcall(cine,"Play") end
    dbg("setup: player=" .. tostring(player ~= nil) .. " boss=" .. tostring(boss ~= nil) .. " katana=" .. tostring(attached))
end
function Tick(dt)
    if not started or handed_off then return end
    hide_hud_for_intro()
    ensure_player_possessed()
    if not boss then boss = first_by_tag("Boss") or first_by_class("ABossEnemyCharacter") end
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
        if Game and Game.ExitCutscene then Game.ExitCutscene() end
        if cine then rcall(cine,"Stop") end
        if boss then
            if Animation and Animation.PlayOnActor then Animation.PlayOnActor(boss, BOSS_IDLE, true) end
            if Combat and Combat.SetTeam then Combat.SetTeam(boss, 2) end
        end
        show_boss_ui_after_intro()
    end
end

function EndPlay()
    restore_hud_after_intro()
end
