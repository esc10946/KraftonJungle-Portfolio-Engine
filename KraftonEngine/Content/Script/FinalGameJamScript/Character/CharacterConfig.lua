local Config = {}

Config.LMB = "LeftMouseButton"
Config.RMB = "RightMouseButton"
Config.MOVE_KEYS = { "W", "A", "S", "D" }

Config.WEAPON_MESH_PATH = "Content/Data/Weapon/JapanSword/Katana_StaticMesh.uasset"
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
Config.SUCCESS_PARRY_MONTAGE_PATH = "Content/Montages/Twin_Cross_Montage.uasset"
Config.SUCCESS_PARRY_SECTION = "Cross"
Config.PARRY_PARTICLE_PATH = "Content/Data/FGJ_Character/Particle/ParryParticle.uasset"
Config.PARRY_PARTICLE_DURATION = 0.5
Config.RPARRY_PARTICLE_SOCKET = "RH_Particle"
Config.LPARRY_PARTICLE_SOCKET = "LH_Particle"

Config.HIT_MONTAGE_PATHS = {
    F = "Content/Montages/Twin_Hit_F_Montage.uasset",
    B = "Content/Montages/Twin_Hit_B_Montage.uasset",
    L = "Content/Montages/Twin_Hit_L_Montage.uasset",
    R = "Content/Montages/Twin_Hit_R_Montage.uasset",
}

return Config
