local Config = {}

Config.LMB = "LeftMouseButton"
Config.RMB = "RightMouseButton"
Config.MOVE_KEYS = { "W", "A", "S", "D" }

Config.WEAPON_MESH_PATH = "Content/Data/FGJ_Character/Weapon/Katana_StaticMesh.uasset"
Config.RWEAPON_SOCKET = "RH_Socket"
Config.LWEAPON_SOCKET = "LH_Socket"

Config.GROUND_SPEED_VAR = "GroundSpeed"

Config.ATTACK_PLAY_RATE = 1.4
Config.ATTACK_MONTAGE_PATHS = {
    "Content/Montages/Attack1_Montage.uasset",
    "Content/Montages/Attack2_Montage.uasset",
    "Content/Montages/Attack3_Montage.uasset",
    "Content/Montages/Attack4_Montage.uasset",
}

Config.DEFENSE_IDLE_MONTAGE_PATH = "Content/Montages/DefenseIdle_Montage.uasset"
Config.SUCCESS_PARRY_MONTAGE_PATH = "Content/Montages/SuccessParry_Montage.uasset"

Config.HIT_MONTAGE_PATHS = {
    F = "Content/Montages/Hit_F_Montage.uasset",
    B = "Content/Montages/Hit_B_Montage.uasset",
    L = "Content/Montages/Hit_L_Montage.uasset",
    R = "Content/Montages/Hit_R_Montage.uasset",
}

return Config
