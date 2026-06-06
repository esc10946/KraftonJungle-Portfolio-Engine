-- =============================================================================
--  BossIntroDirector.lua — Samurai 보스 등장 컷씬 + 전투 핸드오프
-- =============================================================================
--  보스/시네마틱 액터는 BeginPlay 순서에 따라 아직 태그 등록 전일 수 있어 Tick 에서
--  지연 획득한다. 컷씬 진입(입력차단) → 칼 장착 → 워크인 + 발걸음 쉐이크 + 시네마틱
--  공전 → 인트로 종료 → 입력 복귀 + 적대 팀 전환으로 AI 전투 시작.
-- =============================================================================

local INTRO_TIME     = 6.0
local SHAKE_INTERVAL = 0.45   -- 발걸음 셰이크 기준 간격(아래에서 ±무작위로 비주기화)
local STOP_SHORT     = 7.0

local KATANA    = "Content/Data/FGJ_Character/Weapon/Katana_StaticMesh.uasset"
local BOSS_WALK = "Content/Data/Samura2/source/Standing Walk Forward_mixamo_com.uasset"
local BOSS_IDLE = "Content/Data/Samura2/source/Standing Idle_mixamo_com.uasset"
local HAND_BONES = { "hand_r", "RightHand", "mixamorig:RightHand", "weapon_r" }

local boss, player, cine
local t           = 0.0
local started     = false
local setup_done  = false
local handed_off  = false
local shake_accum = 0.0
local shake_next  = SHAKE_INTERVAL
local walk_from, walk_to

local function dbg(m) if print then print("[BossIntro] " .. tostring(m)) end end

local function first_by_tag(tag)
    if World and World.FindActorsByTag then
        local l = World.FindActorsByTag(tag)
        if l and #l > 0 then return l[1] end
    end
    return nil
end

local function first_by_class(cn)
    if World and World.FindActorsByClass then
        local l = World.FindActorsByClass(cn)
        if l and #l > 0 then return l[1] end
    end
    return nil
end

local function rcall(o, fn, ...)
    if o and Reflection and Reflection.Call then return Reflection.Call(o, fn, ...) end
    return nil
end

local function lerp(a, b, s) return a + (b - a) * s end

function BeginPlay()
    player = first_by_tag("Player")
    if Game and Game.EnterCutscene then Game.EnterCutscene() end
    dbg("BeginPlay player=" .. tostring(player ~= nil))
    started = true
end

local function do_setup()
    setup_done = true
    dbg("boss acquired; setup")

    -- 칼을 오른손(hand_r)에 장착.
    local attached = false
    if Equipment and Equipment.AttachStaticMeshToBone then
        for _, bone in ipairs(HAND_BONES) do
            local w = Equipment.AttachStaticMeshToBone(boss, KATANA, bone, Vec3(1.0, 1.0, 1.0))
            if w ~= nil then attached = true; dbg("katana on " .. bone); break end
        end
    end
    if not attached then dbg("katana NOT attached") end

    if Locomotion and Locomotion.SetEnabled then Locomotion.SetEnabled(boss, false) end
    if Animation and Animation.PlayOnActor then Animation.PlayOnActor(boss, BOSS_WALK, true) end

    pcall(function()
        local p = boss.Location
        walk_from = Vec3(p.X, p.Y, p.Z)
        walk_to   = Vec3(p.X, p.Y, p.Z)
        if player then
            local pp = player.Location
            local dx, dy = pp.X - p.X, pp.Y - p.Y
            local d = math.sqrt(dx * dx + dy * dy)
            if d > 0.001 then
                local k = math.max(0.0, d - STOP_SHORT) / d
                walk_to = Vec3(p.X + dx * k, p.Y + dy * k, p.Z)
            end
        end
    end)

    -- 시네마틱 카메라 시작(씬에서 자동재생이지만 명시적으로도 트리거).
    cine = first_by_class("ACameraCinematicActor")
    if cine then
        local wpc = rcall(cine, "GetWaypointCount")
        dbg("cine found waypoints=" .. tostring(wpc))
        -- 좌표 추종 대상을 보스로 직접 연결 — 씬의 ObjectId 참조에 의존하지 않고 런타임에 확정.
        -- 이래야 릭(과 웨이포인트 LookAt 폴백)이 걸어오는 보스를 실제로 따라간다.
        rcall(cine, "SetCoordinateTarget", boss)
        rcall(cine, "Play")
    else
        dbg("cine NOT found")
    end
end

function Tick(dt)
    if not started then return end

    if not boss then
        boss = first_by_tag("Boss") or first_by_tag("Samurai") or first_by_class("ABossEnemyCharacter")
    end
    if boss and not setup_done then do_setup() end

    if handed_off then return end
    t = t + dt

    if t >= INTRO_TIME then
        handed_off = true
        dbg("handoff t=" .. string.format("%.1f", t))
        if Game and Game.ExitCutscene then Game.ExitCutscene() end
        if cine then rcall(cine, "Stop") end
        if boss then
            if Animation and Animation.PlayOnActor then Animation.PlayOnActor(boss, BOSS_IDLE, true) end
            if Locomotion and Locomotion.SetEnabled then Locomotion.SetEnabled(boss, true) end
            if Combat and Combat.SetTeam then Combat.SetTeam(boss, 2) end
            dbg("combat started")
        else
            dbg("handoff but boss STILL nil")
        end
        return
    end

    pcall(function()
        if boss and walk_from and walk_to then
            local s = math.min(1.0, t / INTRO_TIME)
            boss.Location = Vec3(lerp(walk_from.X, walk_to.X, s),
                                 lerp(walk_from.Y, walk_to.Y, s),
                                 lerp(walk_from.Z, walk_to.Z, s))
        end
    end)

    -- 발걸음 셰이크: 간격과 세기를 매 발마다 무작위로 흔들어 "똑딱이는" 메트로놈 느낌을 없앤다.
    -- (셰이크 자체도 인스턴스별 진폭/주파수 변조 + 감쇠 엔벨로프라 한 발 한 발 묵직하게 쿵 한다.)
    shake_accum = shake_accum + dt
    if shake_accum >= shake_next then
        shake_accum = 0.0
        shake_next  = SHAKE_INTERVAL * (0.75 + math.random() * 0.6) -- ≈0.34~0.65s 비주기
        if Camera and Camera.Shake then
            Camera.Shake(0.5 + math.random() * 0.5)                 -- 0.5~1.0 세기 변주(과하지 않게)
        end
    end
end
