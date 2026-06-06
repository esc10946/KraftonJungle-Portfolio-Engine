#!/usr/bin/env python3
# One-shot scene builder: injects the Samurai boss encounter into GamePlay.Scene.
# Idempotent-ish: it strips any previously-injected actors (by tag) before re-adding.
import json, copy, sys, io

ROOT = r'C:\Users\jungle\RiderProjects\Jungle_Week14_Team6\KraftonEngine\Content\Scene\Game'
GAMEPLAY = ROOT + r'\GamePlay.Scene'
SANDBOX  = ROOT + r'\AI_CombatSandbox.Scene'

# Samura2 imported anim assets (bound to Samura2_Skeleton).
SRC = "Content/Data/Samura2/source/"
IDLE   = SRC + "Standing Idle_mixamo_com.uasset"
WALK   = SRC + "Standing Walk Forward_mixamo_com.uasset"
RUN    = SRC + "Standing Run Forward_mixamo_com.uasset"
JUMP   = SRC + "Standing Jump_mixamo_com.uasset"
ATTACK = SRC + "Standing Melee Combo Attack Ver. 1_mixamo_com.uasset"
MESH   = SRC + "Samura2_SkeletalMesh.uasset"

# World placement (Z-up; player at [-22.5,31.7,66.6] facing +X).
BOSS_LOC = [-8.0, 31.731049, 66.614586]
BOSS_ROT = [0.0, 180.0, 0.0]          # face -X toward player
BOSS_SCALE = [1.6, 1.6, 1.6]          # heavy / oversized

BOSS_ID   = 100
CINE_ID   = 200
DIR_ID    = 300
PLAYER_COMP_ID = 400

def load(p):
    with io.open(p, 'r', encoding='utf-8') as f:
        return json.load(f)

def remap_ids(node, base):
    """Reassign every ObjectId in a cloned subtree to fresh unique ids (base+counter),
    and rewrite any {'ObjectId':N} references that point inside the subtree."""
    old_ids = []
    def collect(o):
        if isinstance(o, dict):
            if isinstance(o.get('ObjectId'), int):
                old_ids.append(o['ObjectId'])
            for v in o.values(): collect(v)
        elif isinstance(o, list):
            for v in o: collect(v)
    collect(node)
    mapping = {}
    n = base
    for oid in old_ids:
        if oid not in mapping:
            mapping[oid] = n
            n += 1
    def rewrite(o):
        if isinstance(o, dict):
            # primary id
            if isinstance(o.get('ObjectId'), int) and o['ObjectId'] in mapping:
                o['ObjectId'] = mapping[o['ObjectId']]
            for v in o.values(): rewrite(v)
        elif isinstance(o, list):
            for v in o: rewrite(v)
    rewrite(node)
    return mapping, n

def find_comp(actor, classname):
    out = []
    def walk(c):
        if c.get('ClassName') == classname: out.append(c)
        for ch in c.get('Children', []): walk(ch)
    if actor.get('RootComponent'): walk(actor['RootComponent'])
    for c in actor.get('NonSceneComponents', []):
        if c.get('ClassName') == classname: out.append(c)
    return out

def main():
    gp = load(GAMEPLAY)
    sb = load(SANDBOX)

    # Drop any previously-injected actors so re-runs stay clean.
    INJECT_TAGS = ("Enemy Boss Samurai", "CinematicIntro", "BossIntroDirector")
    def is_injected(a):
        t = a.get('Properties', {}).get('PendingTagsString', '') or ''
        return any(tag in t for tag in INJECT_TAGS)
    gp['Actors'] = [a for a in gp['Actors'] if not is_injected(a)]

    # ---- 1) BOSS (clone sandbox boss, remap, mutate) ----
    boss = None
    for a in sb['Actors']:
        if a.get('ClassName') == 'ABossEnemyCharacter':
            boss = copy.deepcopy(a); break
    assert boss, "no ABossEnemyCharacter in sandbox"
    remap_ids(boss, BOSS_ID + 1)   # subtree ids -> 101.., actor id set below
    boss['ObjectId'] = BOSS_ID
    boss['Name'] = 'ABossEnemyCharacter_Samurai'
    P = boss['Properties']
    P['PendingActorLocation'] = BOSS_LOC
    P['PendingActorRotation'] = BOSS_ROT
    P['PendingActorScale']    = BOSS_SCALE
    P['PendingTagsString']    = 'Enemy Boss Samurai'
    P['bAutoPossessAI']       = True
    # 두뇌 = 코드 내장 세키로 보스 Behavior Tree(@SekiroBoss). BrainBehaviorTreeFile 이 지정되면
    # ConfigureBrainDriver 가 BT 를 최우선으로 켜고 Lua(enemy_brain.lua)는 끈다.
    P['BrainBehaviorTreeFile'] = '@SekiroBoss'

    # Skeletal mesh -> Samura2; drop sandbox material slots (let mesh provide its own).
    for skm in find_comp(boss, 'USkeletalMeshComponent'):
        skm['Properties']['SkeletalMeshPath'] = MESH
        skm['Properties'].pop('MaterialSlots', None)
        # plant feet: keep mesh local offset, identity scale (actor scale handles size)
        skm['Properties']['RelativeTransform.Location'] = [0.0, 0.0, -1.25]
        skm['Properties']['RelativeTransform.Scale']    = [1.0, 1.0, 1.0]

    # Team Neutral during cutscene (director flips to Enemy when combat starts).
    for cs in find_comp(boss, 'UCombatStateComponent'):
        cs['Properties']['Team'] = 0

    # Three Sekiro-style phases.
    for ph in find_comp(boss, 'UPhaseComponent'):
        ph['Properties']['Phases'] = [
            {"Phase": 1, "PhaseName": "Phase1"},
            {"Phase": 2, "PhaseName": "Phase2"},
            {"Phase": 3, "PhaseName": "Phase3"},
        ]
        ph['Properties']['InitialPhase'] = 1
        ph['Properties']['CurrentPhase'] = 1

    # Locomotion driver (visible idle/walk/run/jump/attack + footstep camera shake).
    loco = {
        "ClassName": "UCharacterLocomotionComponent",
        "ObjectId": BOSS_ID + 50,   # 150, outside the remapped subtree range
        "Properties": {
            "IdleAnimPath": IDLE, "WalkAnimPath": WALK, "RunAnimPath": RUN,
            "JumpAnimPath": JUMP, "AttackAnimPath": ATTACK,
            "WalkSpeedThreshold": 0.15, "RunSpeedThreshold": 4.0,
            "bEnableFootstepShake": True, "FootstepShakeScale": 1.4,
            "FootfallsPerCycle": 2, "RunShakeMultiplier": 1.7,
            "bDriverEnabled": False,
            "bIsActive": True, "bAutoActivate": True, "bTickEnable": True,
            "bEditorOnly": False, "bHiddenInComponentTree": False,
        },
    }
    boss['NonSceneComponents'].append(loco)

    # ---- 2) PLAYER combat components (so the boss can target/engage it) ----
    player = next(a for a in gp['Actors'] if a.get('ClassName') == 'ALuaCharacter')
    if not find_comp(player, 'UHealthComponent'):
        player['NonSceneComponents'].append({
            "ClassName": "UHealthComponent", "ObjectId": PLAYER_COMP_ID,
            "Properties": {"MaxHealth": 150.0, "CurrentHealth": 150.0,
                           "bInitializeFullHealthOnBeginPlay": True, "bInvincible": False,
                           "bIsActive": True, "bAutoActivate": True, "bTickEnable": True},
        })
    if not find_comp(player, 'UCombatStateComponent'):
        player['NonSceneComponents'].append({
            "ClassName": "UCombatStateComponent", "ObjectId": PLAYER_COMP_ID + 1,
            "Properties": {"Team": 1, "MaxPoise": 80.0, "CurrentPoise": 80.0,
                           "bDamageEnabled": True, "bIsActive": True,
                           "bAutoActivate": True, "bTickEnable": True},
        })
    if 'Player' not in (player['Properties'].get('PendingTagsString','') or ''):
        existing = player['Properties'].get('PendingTagsString','') or ''
        player['Properties']['PendingTagsString'] = (existing + ' Player').strip()

    # ---- 3) CINEMATIC actor (orbit the boss, low->high) ----
    def wp(i, off):
        return {
            "ClassName": "UCinematicWaypointComponent",
            "ObjectId": CINE_ID + 10 + i,
            "Properties": {
                "AttachSocketName": "Name_None",
                "RelativeTransform.Location": off,
                "RelativeTransform.Scale": [1.0, 1.0, 1.0],
                "CachedEditRotator": [0.0, 0.0, 0.0],
                "RotMode": 1,  # LookAtTarget
                "LookAtActor": {"ObjectId": BOSS_ID},
                "SequenceIndex": i, "FOVOverride": 0.0,
                "SegmentSeconds": 0.0, "DwellSeconds": 0.0,
                "DecelIn": 0.0, "AccelOut": 0.0,
            },
            "Children": [],
        }
    # offsets relative to rig (at boss); arc from player-side low -> high
    offsets = [
        [-10.0,  0.0, 0.5],
        [ -9.0, -6.0, 2.0],
        [ -3.0,-10.0, 3.5],
        [  4.0, -8.0, 5.5],
        [  8.0, -2.0, 7.0],
    ]
    cine = {
        "ClassName": "ACameraCinematicActor", "Name": "ACameraCinematicActor_Intro",
        "ObjectId": CINE_ID, "NonSceneComponents": [],
        "Properties": {
            "PendingActorLocation": BOSS_LOC, "PendingActorRotation": [0.0,0.0,0.0],
            "PendingActorScale": [1.0,1.0,1.0], "PendingActorVisible": True,
            "PendingTagsString": "CinematicIntro", "bNeedsTick": True,
            "CoordinateTarget": {"ObjectId": BOSS_ID},
            "Duration": 6.0, "DefaultFOV": 1.0471975512,
            "bLoop": False, "bSmoothPath": True, "bPlayOnBeginPlay": True,
            "BlendInTime": 0.0, "BlendOutTime": 0.5,
            "PathColor": [0.2, 0.8, 1.0, 1.0],
        },
        "RootComponent": {
            "ClassName": "USceneComponent", "ObjectId": CINE_ID + 1,
            "Properties": {
                "AttachSocketName": "Name_None",
                "RelativeTransform.Location": BOSS_LOC,
                "RelativeTransform.Scale": [1.0,1.0,1.0],
                "CachedEditRotator": [0.0,0.0,0.0],
            },
            "Children": [
                {"ClassName": "UCineCameraComponent", "ObjectId": CINE_ID + 2,
                 "Properties": {"AttachSocketName": "Name_None",
                                "RelativeTransform.Location": [0.0,0.0,0.0],
                                "RelativeTransform.Scale": [1.0,1.0,1.0],
                                "CachedEditRotator": [0.0,0.0,0.0]},
                 "Children": []},
            ] + [wp(i, off) for i, off in enumerate(offsets)],
        },
    }

    # ---- 4) DIRECTOR actor (clone existing AEmptyActor structure) ----
    src_empty = next(a for a in gp['Actors'] if a.get('ClassName') == 'AEmptyActor')
    director = copy.deepcopy(src_empty)
    remap_ids(director, DIR_ID + 1)
    director['ObjectId'] = DIR_ID
    director['Name'] = 'AEmptyActor_BossDirector'
    dp = director['Properties']
    dp['PendingActorLocation'] = BOSS_LOC
    dp['PendingTagsString'] = 'BossIntroDirector'
    lua = find_comp(director, 'ULuaScriptComponent')
    assert lua, "director clone has no ULuaScriptComponent"
    lua[0]['Properties']['ScriptFile'] = 'Game/BossIntroDirector.lua'

    gp['Actors'].extend([boss, cine, director])

    # ---- validate id uniqueness ----
    seen = {}
    dup = []
    def chk(o, owner):
        if isinstance(o, dict):
            oid = o.get('ObjectId')
            # Only object *definitions* (have a ClassName) own an id; bare {"ObjectId":N}
            # dicts are references (CoordinateTarget / LookAtActor) and don't conflict.
            if isinstance(oid, int) and 'ClassName' in o:
                if oid in seen: dup.append((oid, seen[oid], owner))
                else: seen[oid] = owner
            for v in o.values(): chk(v, owner)
        elif isinstance(o, list):
            for v in o: chk(v, owner)
    for a in gp['Actors']:
        chk(a, a.get('Name') or a.get('ClassName'))
    if dup:
        print("DUPLICATE OBJECT IDS:", dup); sys.exit(1)

    with io.open(GAMEPLAY, 'w', encoding='utf-8') as f:
        json.dump(gp, f, indent=2)

    print("OK. actors now:", [(a.get('ClassName'), a.get('ObjectId')) for a in gp['Actors']])
    print("boss subtree ids unique; total actors:", len(gp['Actors']))

if __name__ == '__main__':
    main()
