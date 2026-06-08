local Config = {}

Config.LMB = "LeftMouseButton"
Config.RMB = "RightMouseButton"
Config.LOCK_ON = "MiddleMouseButton"
Config.EXECUTION_KEY = "E"
Config.GAMEPAD_ATTACK = "GamepadRightShoulder"
Config.GAMEPAD_GUARD = "GamepadLeftShoulder"
Config.GAMEPAD_LOCK_ON = "GamepadRightThumbstick"
Config.GAMEPAD_MOVE_X_AXIS = "GamepadLeftX"
Config.GAMEPAD_MOVE_Y_AXIS = "GamepadLeftY"
Config.GAMEPAD_LOOK_X_AXIS = "GamepadRightX"
Config.MOVE_FORWARD_KEY = "W"
Config.MOVE_LEFT_KEY = "A"
Config.MOVE_BACK_KEY = "S"
Config.MOVE_RIGHT_KEY = "D"
Config.MOVE_KEYS = {
    Config.MOVE_FORWARD_KEY,
    Config.MOVE_LEFT_KEY,
    Config.MOVE_BACK_KEY,
    Config.MOVE_RIGHT_KEY,
}

Config.WEAPON_MESH_PATH = "Content/Data/Weapon/JapanSword/New_Katana_StaticMesh.uasset"
Config.RWEAPON_SOCKET = "RH_Socket"
Config.LWEAPON_SOCKET = "LH_Socket"

Config.GROUND_SPEED_VAR = "GroundSpeed"

Config.ATTACK_PLAY_RATE = 1.7
Config.ATTACK_MONTAGE_PATHS = {
    "Content/Montages/Twin_Attack1_Montage.uasset",
    "Content/Montages/Twin_Attack2_Montage.uasset",
    "Content/Montages/Twin_Attack3_Montage.uasset",
    "Content/Montages/Twin_Attack4_Montage.uasset",
}
Config.DEFENSE_IDLE_MONTAGE_PATH = "Content/Montages/Twin_Defense_Montage.uasset"
Config.COUNTER_MONTAGE_PATH = "Content/Montages/Twin_Cross_Montage.uasset"
Config.COUNTER_SECTION = "Cross"
Config.COUNTER_CAMERA_SHAKE_PATH = "Content/Data/FGJ_Character/CameraWork/CrossShake.uasset"
Config.COUNTER_CAMERA_SHAKE_SCALE = 1.0
Config.COUNTER_PARTICLE_PATH = "Content/Particle System/Parry.uasset"
Config.COUNTER_PARTICLE_DURATION = 1.5
Config.RCOUNTER_PARTICLE_SOCKET = "RH_Particle"
Config.LCOUNTER_PARTICLE_SOCKET = "LH_Particle"
Config.SWORD_TRAIL_PARTICLE_PATH = "Content/Particle System/SwordTrail.uasset"
Config.RSWORD_TRAIL_PARTICLE_SOCKET = "RH_Particle"
Config.LSWORD_TRAIL_PARTICLE_SOCKET = "LH_Particle"
Config.COUNTER_IMPACT_PARTICLE_SCALE = 0.05
Config.COUNTER_PARRY_SOUND_KEY = "Counter_Parry1"
Config.COUNTER_PARRY_SOUND_PATH = "Parry1.wav"
Config.COUNTER_PARRY_SOUND_VOLUME = 0.6
Config.COUNTER_DEBUG_LOGS = false
Config.COUNTER_REPOSITION_ENABLED = true
Config.COUNTER_REPOSITION_DISTANCE = 2.0
Config.COUNTER_REPOSITION_DURATION = 0.18
Config.COUNTER_PAWN_OVERLAP_RESPONSE = 1
Config.COUNTER_SLOMO_DURATION = 0.2
Config.COUNTER_SLOMO_TIME_DILATION = 0.25
Config.COUNTER_INPUT_BUFFER_SECONDS = 0.2
Config.COUNTER_INPUT_DEFLECT_WINDOW_SECONDS = 0.25

Config.EXECUTION_PLAYER_MONTAGE_PATH = "Content/Montages/Twin_Execute_Montage.uasset"
Config.EXECUTION_BOSS_MONTAGE_PATH = "Content/Montages/ExecutedA_Montage.uasset"
Config.EXECUTION_BOSS_CLASSES = {
    "BossEnemyCharacter",
    "ABossEnemyCharacter",
}
Config.EXECUTION_BOSS_TAG = "Boss"
Config.EXECUTION_MAX_DISTANCE = 5.0
Config.EXECUTION_ALIGN_ENABLED = true
Config.EXECUTION_PLAY_RATE = 1.0
Config.EXECUTION_DEBUG_LOGS = true

Config.HIT_MONTAGE_PATHS = {
    F = "Content/Montages/Twin_Hit_F_Montage.uasset",
    B = "Content/Montages/Twin_Hit_B_Montage.uasset",
    L = "Content/Montages/Twin_Hit_L_Montage.uasset",
    R = "Content/Montages/Twin_Hit_R_Montage.uasset",
}

return Config
