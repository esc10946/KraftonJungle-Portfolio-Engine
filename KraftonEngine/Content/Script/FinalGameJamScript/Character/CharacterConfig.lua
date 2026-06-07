local Config = {}

Config.LMB = "LeftMouseButton"
Config.RMB = "RightMouseButton"
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
Config.COUNTER_PARTICLE_PATH = "Content/Data/FGJ_Character/Particle/ParryParticle.uasset"
Config.COUNTER_PARTICLE_DURATION = 0.5
Config.RCOUNTER_PARTICLE_SOCKET = "RH_Particle"
Config.LCOUNTER_PARTICLE_SOCKET = "LH_Particle"
Config.COUNTER_REPOSITION_ENABLED = true
Config.COUNTER_REPOSITION_DISTANCE = 2.0
Config.COUNTER_REPOSITION_DURATION = 0.18
Config.COUNTER_PAWN_OVERLAP_RESPONSE = 1
Config.COUNTER_SLOMO_DURATION = 0.2
Config.COUNTER_SLOMO_TIME_DILATION = 0.25

Config.HIT_MONTAGE_PATHS = {
    F = "Content/Montages/Twin_Hit_F_Montage.uasset",
    B = "Content/Montages/Twin_Hit_B_Montage.uasset",
    L = "Content/Montages/Twin_Hit_L_Montage.uasset",
    R = "Content/Montages/Twin_Hit_R_Montage.uasset",
}

return Config
