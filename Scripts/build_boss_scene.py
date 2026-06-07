#!/usr/bin/env python3
# One-shot scene builder: injects the Samurai boss encounter into GamePlay.Scene.
# Idempotent-ish: it strips any previously-injected actors (by tag) before re-adding.
import json, copy, sys, io
from pathlib import Path

# Boss intro BP intentionally enters cutscene after PostStartMatch, not BeginPlay, to avoid GameMode phase overwrite.

# Resolve from this script location so the builder works on any clone path.
REPO_ROOT = Path(__file__).resolve().parents[1]
ROOT = REPO_ROOT / 'KraftonEngine' / 'Content' / 'Scene' / 'Game'
GAMEPLAY = ROOT / 'GamePlay.Scene'
SANDBOX  = ROOT / 'AI_CombatSandbox.Scene'

# Samura2 imported anim assets (bound to Samura2_Skeleton).
SRC = "Content/Data/Samura2/source/"
IDLE   = SRC + "Standing Idle_mixamo_com.uasset"
WALK   = SRC + "Standing Walk Forward_mixamo_com.uasset"
RUN    = SRC + "Standing Run Forward_mixamo_com.uasset"
JUMP   = SRC + "Standing Jump_mixamo_com.uasset"
ATTACK = SRC + "Standing Melee Combo Attack Ver. 1_mixamo_com.uasset"
ATTACK_HIGH = SRC + "Standing Melee Attack 360 High_mixamo_com.uasset"
ATTACK_LOW  = SRC + "Standing Melee Attack 360 Low_mixamo_com.uasset"
ATTACK_DOWN = SRC + "Standing Melee Attack Downward_mixamo_com.uasset"
ATTACK_HORZ = SRC + "Standing Melee Attack Horizontal_mixamo_com.uasset"
JUMP_ATTACK = SRC + "Standing Melee Run Jump Attack_mixamo_com.uasset"
MESH   = SRC + "Samura2_SkeletalMesh.uasset"
BT_ASSET = "Content/BehaviorTree/BossSamuraiSekiroBT.uasset"
BOSS_INTRO_BP = "Content/Blueprint/BossIntroDirectorBP.uasset"

# World placement (Z-up; player at [-22.5,31.7,66.6] facing +X).
BOSS_LOC = [-8.0, 31.731049, 66.614586]
BOSS_ROT = [0.0, 180.0, 0.0]          # face -X toward player
BOSS_SCALE = [1.6, 1.6, 1.6]          # heavy / oversized

BOSS_ID   = 100
CINE_ID   = 200
DIR_ID    = 300
PLAYER_COMP_ID = 400
NAV_ID = 500

def load(p):
    with io.open(str(p), 'r', encoding='utf-8') as f:
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

    # ---- 0) NAVIGATION BOUNDS (explicit boss arena nav volume) ----
    # AI_Interaction.Scene uses an explicit ANavMeshBoundsVolume. GamePlay must also
    # bound the boss/player combat area; relying on GridNavMesh fallback over the full
    # static map makes start/end projection and oversized boss path queries unreliable.
    gp['Actors'] = [a for a in gp['Actors'] if a.get('Name') != 'BossArena_NavBounds_Main'
                    and 'BossArenaNavBounds' not in (a.get('Properties', {}).get('PendingTagsString', '') or '')]
    nav_center = [(BOSS_LOC[0] + -22.538132) / 2.0, BOSS_LOC[1], BOSS_LOC[2]]
    nav_bounds = {
        'ClassName': 'ANavMeshBoundsVolume', 'Name': 'BossArena_NavBounds_Main',
        'ObjectId': NAV_ID, 'NonSceneComponents': [],
        'Properties': {
            'PendingActorLocation': nav_center, 'PendingActorRotation': [0.0, 0.0, 0.0],
            'PendingActorScale': [1.0, 1.0, 1.0], 'PendingActorVisible': True,
            'PendingTagsString': 'BossArenaNavBounds', 'bNeedsTick': True,
        },
        'RootComponent': {
            'ClassName': 'UBoxComponent', 'ObjectId': NAV_ID + 1,
            'Properties': {
                'AttachSocketName': 'Name_None', 'BoxExtent': [34.0, 24.0, 10.0],
                'CachedEditRotator': [0.0, 0.0, 0.0], 'CenterOfMassOffset': [0.0, 0.0, 0.0],
                'CollisionEnabled': 0, 'Mass': 1.0, 'ObjectType': 0,
                'RelativeTransform.Location': nav_center, 'RelativeTransform.Scale': [1.0, 1.0, 1.0],
                'ResponseContainer': {'Responses[0]': 2, 'Responses[1]': 2, 'Responses[2]': 2, 'Responses[3]': 2, 'Responses[4]': 2},
                'ShapeColor': [0.0, 0.5, 1.0, 0.25], 'bAutoActivate': True,
                'bCanEverAffectNavigation': True, 'bCastShadow': False, 'bCastShadowAsTwoSided': False,
                'bDrawOnlyIfSelected': False, 'bEditorOnly': False, 'bEnableCCD': False,
                'bGenerateOverlapEvents': False, 'bHiddenInComponentTree': False, 'bIsActive': True,
                'bIsVisible': True, 'bKinematic': False, 'bSimulatePhysics': False, 'bTickEnable': True,
            },
            'Children': [],
        },
    }
    gp['Actors'].append(nav_bounds)
    nav = gp.setdefault('WorldSettings', {}).setdefault('Navigation', {})
    nav.update({'bUseNavigationData': True, 'bAutoRebuildOnPathRequest': True, 'bEnableRuntimeAutoRebuild': True,
                'bAllowRuntimeFallback': False, 'AgentRadius': 1.2, 'AgentHeight': 4.0,
                'AgentStepHeight': 0.6, 'AgentMaxClimbHeight': 0.6, 'AgentMaxDropHeight': 0.8,
                'ProjectionUp': 8.0, 'ProjectionDown': 12.0, 'CellSize': 0.75})

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
    P['TargetSearchRange']    = 28.0
    P['DefaultAttackRange']   = 3.2
    P['TurnSpeed']            = 720.0
    P['GapCloserRangeScale']  = 3.2
    P['GapCloserDashScale']   = 1.65
    P['SquadMaxSimultaneousAttackers'] = 1
    # 두뇌 = 보스 전용 고도화 Behavior Tree. BT 런타임에 SelectAttack/ReactiveDeflect/
    # TacticalMove 노드를 추가했으므로 페이즈·거리·성향·후딜·반복 억제 기반 선택이 BT 안에서 돈다.
    # enemy_brain.lua 는 BT 자산 로드 실패 시 폴백으로만 남긴다.
    P['BrainBehaviorTreeFile'] = BT_ASSET
    P['BrainBlueprintFile'] = ''
    P['BrainScriptFile'] = 'AI/enemy_brain.lua'

    # Skeletal mesh -> Samura2; drop sandbox material slots (let mesh provide its own).
    for skm in find_comp(boss, 'USkeletalMeshComponent'):
        skm['Properties']['SkeletalMeshPath'] = MESH
        skm['Properties'].pop('MaterialSlots', None)
        # plant feet: keep mesh local offset, identity scale (actor scale handles size)
        skm['Properties']['RelativeTransform.Location'] = [0.0, 0.0, -1.25]
        skm['Properties']['RelativeTransform.Scale']    = [1.0, 1.0, 1.0]

    for mv in find_comp(boss, 'UCharacterMovementComponent'):
        mv['Properties']['MaxWalkSpeed'] = 7.2
        mv['Properties']['MaxAcceleration'] = 32.0
        mv['Properties']['JumpZVelocity'] = 8.0
        mv['Properties']['RotationYawRate'] = 900.0
        mv['Properties']['bOrientRotationToMovement'] = True

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

    def atk(name, min_phase, max_phase, min_range, max_range, weight, priority,
            tactic, damage, poise, cooldown, recovery, gap=False, perilous=0,
            angle=80.0, startup=10, active=5, recover_frames=22,
            requires_prev=False, prev='None'):
        return {
            "AttackName": name,
            "Montage": None,
            "MinPhase": min_phase, "MaxPhase": max_phase,
            "MinRange": min_range, "MaxRange": max_range,
            "MaxVerticalDelta": 2.25,
            "MaxAbsAngle": angle,
            "Cooldown": cooldown,
            "RecoveryTime": recovery,
            "Weight": weight,
            "RepeatWeightScale": 0.28,
            "Damage": damage,
            "PoiseDamage": poise,
            "bRequiresTargetInFront": angle < 100.0,
            "bIsGapCloser": gap,
            "TacticTag": tactic,
            "Priority": priority,
            "bRequiresPreviousAttack": requires_prev,
            "RequiredPreviousAttack": prev,
            "StartupFrames": startup,
            "ActiveFrames": active,
            "RecoveryFrames": recover_frames,
            "PerilousCueFrame": max(3, startup - 5),
            "PerilousType": perilous,
            "bCanBeDeflected": True,
        }

    for ac in find_comp(boss, 'UEnemyAttackComponent'):
        ac['Properties']['GlobalCooldown'] = 0.18
        ac['Properties']['RecentAttackMemory'] = 3
        ac['Properties']['Attacks'] = [
            atk('Samurai_P1_DrawSlash', 1, 1, 0.0, 3.8, 1.45, 1.35, 'Opener', 22.0, 38.0, 1.75, 0.78, False, 0, 72.0, 11, 5, 20),
            atk('Samurai_P1_WideCut', 1, 1, 0.0, 4.4, 1.65, 1.20, 'Pressure', 20.0, 42.0, 1.55, 0.82, False, 0, 100.0, 12, 5, 22),
            atk('Samurai_P1_ClosePunish', 1, 1, 0.0, 2.8, 0.95, 1.75, 'Punish', 26.0, 55.0, 2.6, 0.95, False, 0, 72.0, 9, 4, 20),
            atk('Samurai_P2_FlashStepThrust', 2, 2, 3.0, 7.2, 1.35, 1.65, 'GapCloser', 30.0, 62.0, 2.55, 1.05, True, 1, 64.0, 14, 5, 24),
            atk('Samurai_P2_LowSweep', 2, 2, 0.0, 4.6, 1.60, 1.45, 'Pressure', 28.0, 68.0, 2.2, 1.0, False, 2, 110.0, 13, 5, 24),
            atk('Samurai_P2_AerialDrop', 2, 3, 4.0, 8.4, 1.20, 1.55, 'GapCloser', 34.0, 74.0, 3.0, 1.20, True, 0, 70.0, 15, 5, 26),
            atk('Samurai_P3RelentlessCombo_A', 3, 3, 0.0, 4.5, 1.75, 1.55, 'Combo', 32.0, 78.0, 1.55, 0.92, False, 0, 86.0, 9, 5, 20),
            atk('Samurai_P3RelentlessCombo_B', 3, 3, 0.0, 4.9, 1.25, 1.85, 'Combo', 36.0, 90.0, 1.9, 1.05, False, 0, 95.0, 8, 6, 23, True, 'Samurai_P3RelentlessCombo_A'),
            atk('Samurai_P3MeteorCut', 3, 3, 5.0, 9.5, 1.10, 1.90, 'PhaseChange', 42.0, 105.0, 3.25, 1.35, True, 3, 70.0, 18, 6, 30),
        ]

    # Locomotion driver (visible idle/walk/run/jump/attack + footstep camera shake).
    loco = {
        "ClassName": "UCharacterLocomotionComponent",
        "ObjectId": BOSS_ID + 50,   # 150, outside the remapped subtree range
        "Properties": {
            "IdleAnimPath": IDLE, "WalkAnimPath": WALK, "RunAnimPath": RUN,
            "JumpAnimPath": JUMP, "AttackAnimPath": ATTACK,
            "WalkSpeedThreshold": 0.15, "RunSpeedThreshold": 4.0,
            "bEnableFootstepShake": False, "FootstepShakeScale": 0.0,
            "FootfallsPerCycle": 2, "RunShakeMultiplier": 1.0,
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
    # Blueprint-authored intro director. No C++ camera hook or LuaScriptComponent is required.
    director['NonSceneComponents'] = [{
        'ClassName': 'ULuaBlueprintComponent',
        'ObjectId': DIR_ID + 2,
        'Properties': {
            'BlueprintPath': BOSS_INTRO_BP,
            'bAutoActivate': True,
            'bEditorOnly': False,
            'bHiddenInComponentTree': False,
            'bIsActive': True,
            'bTickEnable': True,
        },
    }]

    # Tick order matters: director moves the boss first, then the cinematic camera
    # evaluates its scene-authored CoordinateTarget in the same frame.
    gp['Actors'].extend([boss, director, cine])

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

    with io.open(str(GAMEPLAY), 'w', encoding='utf-8') as f:
        json.dump(gp, f, indent=2)

    print("OK. actors now:", [(a.get('ClassName'), a.get('ObjectId')) for a in gp['Actors']])
    print("boss subtree ids unique; total actors:", len(gp['Actors']))

if __name__ == '__main__':
    main()

# NOTE: 2026-06-07 fix: GamePlay.Scene must keep actor/root locations synchronized for boss/nav.
