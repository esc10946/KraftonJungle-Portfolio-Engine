local AnimTest = {}

AnimTest.Properties = {
    IdleAnimationPath = {
        Type = "String",
        Default = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Idle.anm.bin",
        Category = "Animation",
    },
    RunAnimationPath = {
        Type = "String",
        Default = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Run.anm.bin",
        Category = "Animation",
    },
    WalkSpeed = {
        Type = "Float",
        Default = 4.0,
        Min = 0.0,
        Max = 100.0,
        Category = "Movement",
    },
    SprintSpeedMultiplier = {
        Type = "Float",
        Default = 1.75,
        Min = 1.0,
        Max = 10.0,
        Category = "Movement",
    },
    LookSensitivityDegrees = {
        Type = "Float",
        Default = 0.12,
        Min = 0.0,
        Max = 5.0,
        Category = "Movement",
    },
    IdleRunBlendTime = {
        Type = "Float",
        Default = 0.18,
        Min = 0.0,
        Max = 5.0,
        Category = "Animation",
    },
    MoveStartSpeedThreshold = {
        Type = "Float",
        Default = 0.1,
        Min = 0.0,
        Max = 100.0,
        Category = "Animation",
    },
    RotateMeshToMovement = {
        Type = "Bool",
        Default = true,
        Category = "Movement",
    },
    AutoConfigureAnimation = {
        Type = "Bool",
        Default = true,
        Category = "Animation",
    },
}

local function clamp_min(value, min_value)
    value = tonumber(value) or min_value
    if value < min_value then
        return min_value
    end
    return value
end

local function get_component(actor, type_name)
    if not actor then
        return nil
    end
    return actor:GetComponent(type_name)
end

local function is_action_active(action_name)
    local input = Engine.API.Input
    return input.WasActionStarted(action_name) or input.IsActionTriggered(action_name)
end

local function axis_length(axis)
    if not axis then
        return 0.0
    end
    return math.sqrt(axis.X * axis.X + axis.Y * axis.Y)
end

function AnimTest.new(component, properties)
    local self = {
        Component = component,
        Actor = component and component:GetOwner() or Actor,
        Properties = properties or {},
        LocomotionStateMachine = nil,
    }
    setmetatable(self, { __index = AnimTest })
    return self
end

function AnimTest:GetProperty(name, fallback)
    local value = self.Properties[name]
    if value == nil then
        return fallback
    end
    return value
end

function AnimTest:CacheComponents()
    self.SkeletalMeshComp = self.Actor and self.Actor:GetSkeletalMeshComponent() or nil
    self.SpringArmComp = self.Actor and self.Actor:GetSpringArmComponent() or nil
    self.CameraComp = self.Actor and self.Actor:GetCameraComponent() or nil

    if not self.SkeletalMeshComp then
        self.SkeletalMeshComp = get_component(self.Actor, "SkeletalMeshComponent")
    end
    if not self.SpringArmComp then
        self.SpringArmComp = get_component(self.Actor, "SpringArmComponent")
    end
    if not self.CameraComp then
        self.CameraComp = get_component(self.Actor, "CameraComponent")
    end
end

function AnimTest:ConfigureLocomotionStateMachine()
    if not self.SkeletalMeshComp then
        return
    end

    local idle_path = self:GetProperty("IdleAnimationPath", "")
    local run_path = self:GetProperty("RunAnimationPath", "")
    if idle_path == "" or run_path == "" then
        return
    end

    local anim_instance = self.SkeletalMeshComp:GetAnimInstance()
    if not anim_instance then
        self.SkeletalMeshComp:UseDefaultAnimInstance()
        anim_instance = self.SkeletalMeshComp:GetAnimInstance()
    end
    if not anim_instance then
        return
    end

    anim_instance:RegisterAnimationPath("Idle", idle_path)
    anim_instance:RegisterAnimationPath("Run", run_path)
    anim_instance:SetAnimFloatParameter("Speed", 0.0)
    anim_instance:SetAnimFloatParameter("GroundSpeed", 0.0)
    anim_instance:SetAnimBoolParameter("bMoving", false)
    anim_instance:SetAnimBoolParameter("bSprinting", false)

    local machine = CreateAnimStateMachineAsset()
    machine:SetEntryState("Idle")
    machine:AddStateWithPath("Idle", "Idle", idle_path, true)
    machine:AddStateWithPath("Run", "Run", run_path, true)

    local threshold = clamp_min(self:GetProperty("MoveStartSpeedThreshold", 0.1), 0.0)
    local blend_time = clamp_min(self:GetProperty("IdleRunBlendTime", 0.18), 0.0)
    machine:AddFloatTransition("Idle", "Run", "Speed", "Greater", threshold, blend_time, 0, "EaseInOut")
    machine:AddFloatTransition("Run", "Idle", "Speed", "LessEqual", threshold, blend_time, 0, "EaseInOut")

    if self.SkeletalMeshComp:UseStateMachine(machine) then
        self.LocomotionStateMachine = machine
    else
        LogWarning("[AnimTest] Failed to activate locomotion state machine")
    end
end

function AnimTest:BeginPlay()
    self:CacheComponents()

    if self:GetProperty("AutoConfigureAnimation", true) then
        self:ConfigureLocomotionStateMachine()
    end
end

function AnimTest:Tick(delta_time)
    self:UpdateLocomotion(delta_time)
end

function AnimTest:UpdateLocomotion(delta_time)
    if not self.Actor or not self.SkeletalMeshComp then
        return
    end

    if delta_time <= 0.0 then
        self:UpdateAnimationParameters(0.0, false, false)
        return
    end

    local input = Engine.API.Input
    local move_axis = input.GetActionAxis2D("Move")
    local look_axis = input.GetActionAxis2D("Look")

    if self.SpringArmComp and look_axis then
        local sensitivity = clamp_min(self:GetProperty("LookSensitivityDegrees", 0.12), 0.0)
        self.SpringArmComp:AddYawInput(look_axis.X * sensitivity)
        self.SpringArmComp:AddPitchInput(-look_axis.Y * sensitivity)
    end

    local move_axis_length = math.min(axis_length(move_axis), 1.0)
    local sprinting = is_action_active("Dash")
    local walk_speed = clamp_min(self:GetProperty("WalkSpeed", 4.0), 0.0)
    local sprint_multiplier = clamp_min(self:GetProperty("SprintSpeedMultiplier", 1.75), 1.0)
    local effective_speed = walk_speed * (sprinting and sprint_multiplier or 1.0)
    local ground_speed = move_axis_length * effective_speed
    local threshold = clamp_min(self:GetProperty("MoveStartSpeedThreshold", 0.1), 0.0)
    local moving = ground_speed > threshold

    if moving then
        local move_direction = self:BuildCameraRelativeMoveDirection(move_axis)
        self.Actor:Add_Actor_World_Offset(move_direction * (ground_speed * delta_time))

        if self:GetProperty("RotateMeshToMovement", true) then
            local atan2 = math.atan2 or math.atan
            local yaw_degrees = math.deg(atan2(move_direction.Y, move_direction.X)) - 90.0
            local mesh_rotation = self.SkeletalMeshComp.Rotation
            local actor_rotation = self.Actor.Rotation
            mesh_rotation.Z = yaw_degrees - actor_rotation.Z
            self.SkeletalMeshComp.Rotation = mesh_rotation
        end
    end

    self:UpdateAnimationParameters(ground_speed, moving, sprinting)
end

function AnimTest:BuildCameraRelativeMoveDirection(move_axis)
    local forward = self.SpringArmComp and self.SpringArmComp.Forward or self.Actor:Get_Actor_Forward()
    local right = self.SpringArmComp and self.SpringArmComp.Right or self.Actor:Get_Actor_Right()

    forward.Z = 0.0
    right.Z = 0.0
    forward = forward:GetSafeNormal()
    right = right:GetSafeNormal()

    local direction = forward * move_axis.Y + right * move_axis.X
    if direction:Normalize() then
        return direction
    end
    return self.Actor:Get_Actor_Forward()
end

function AnimTest:UpdateAnimationParameters(ground_speed, moving, sprinting)
    local anim_instance = self.SkeletalMeshComp and self.SkeletalMeshComp:GetAnimInstance() or nil
    if not anim_instance then
        return
    end

    anim_instance:SetAnimFloatParameter("Speed", ground_speed)
    anim_instance:SetAnimFloatParameter("GroundSpeed", ground_speed)
    anim_instance:SetAnimBoolParameter("bMoving", moving)
    anim_instance:SetAnimBoolParameter("bSprinting", sprinting)
end

return AnimTest
