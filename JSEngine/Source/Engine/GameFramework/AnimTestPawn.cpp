#include "GameFramework/AnimTestPawn.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimationSequenceBase.h"
#include "Animation/AnimStateMachineAsset.h"
#include "Animation/AnimStateMachineNode.h"
#include "Component/CameraComponent.h"
#include "Component/SceneComponent.h"
#include "Component/SkeletalMeshComponent.h"
#include "Component/SpringArmComponent.h"
#include "Core/Logging/Log.h"
#include "Core/ResourceManager.h"
#include "Engine/Input/GameplayInputTypes.h"
#include "GameFramework/PlayerController.h"
#include "Math/Rotator.h"
#include "Math/Utils.h"
#include "Object/Object.h"
#include "Runtime/Engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

DEFINE_CLASS(AAnimTestPawn, APawn)  
REGISTER_FACTORY(AAnimTestPawn)

namespace
{
    const FName NAME_LightAttack("LightAttack");
    const FName NAME_HeavyAttack("HeavyAttack");
    const FName NAME_LightAttackSwing("GwenLightAttackSwing");
    const FName NAME_HeavyAttackSwing("GwenHeavyAttackSwing");

    bool IsActionActive(const FInputActionState* Action)
    {
        return Action &&
            (Action->TriggerEvent == EInputTriggerEvent::Started ||
             Action->TriggerEvent == EInputTriggerEvent::Triggered);
    }

    bool IsActionStarted(const FInputActionState* Action)
    {
        return Action && Action->TriggerEvent == EInputTriggerEvent::Started;
    }

    float AxisLength(const FVector2& Axis)
    {
        return std::sqrt(Axis.X * Axis.X + Axis.Y * Axis.Y);
    }
}

AAnimTestPawn::~AAnimTestPawn()
{
    if (SkeletalMeshComp)
    {
        if (UAnimInstance* AnimInstance = SkeletalMeshComp->GetAnimInstance())
        {
            AnimInstance->SetStateMachineAsset(nullptr);
        }
    }

    if (LocomotionStateMachine)
    {
        UObjectManager::Get().DestroyObject(LocomotionStateMachine);
        LocomotionStateMachine = nullptr;
    }
}

void AAnimTestPawn::InitDefaultComponents()
{
    SceneRoot = AddComponent<USceneComponent>();
    SetRootComponent(SceneRoot);

    SkeletalMeshComp = AddComponent<USkeletalMeshComponent>();
    SkeletalMeshComp->AttachToComponent(SceneRoot);
    SkeletalMeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
    SkeletalMeshComp->SetRelativeScale(FVector(0.01f, 0.01f, 0.01f));

    SpringArmComp = AddComponent<USpringArmComponent>();
    SpringArmComp->AttachToComponent(SceneRoot);
    SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 1.6f));
    SpringArmComp->SetTargetArmLength(5.0f);
    SpringArmComp->SetSocketOffset(FVector(0.0f, 0.0f, 0.25f));

    CameraComp = AddComponent<UCameraComponent>();
    CameraComp->AttachToComponent(SpringArmComp);

    AddTag("AnimTestPawn");
    LoadConfiguredSkeletalMesh();
}

void AAnimTestPawn::BeginPlay()
{
    APawn::BeginPlay();

    LoadConfiguredSkeletalMesh();
    if (bAutoConfigureAnimation)
    {
        ConfigureLocomotionStateMachine();
    }
}

void AAnimTestPawn::Tick(float DeltaTime)
{
    APawn::Tick(DeltaTime);
    UpdateLocomotion(DeltaTime);
}

void AAnimTestPawn::Serialize(FArchive& Ar)
{
    APawn::Serialize(Ar);

    Ar << "SkeletalMeshPath" << SkeletalMeshPath;
    Ar << "IdleAnimationPath" << IdleAnimationPath;
    Ar << "IntoRunAnimationPath" << IntoRunAnimationPath;
    Ar << "RunAnimationPath" << RunAnimationPath;
    Ar << "HomeguardAnimationPath" << HomeguardAnimationPath;
    Ar << "LightAttackAnimationPath" << LightAttackAnimationPath;
    Ar << "HeavyAttackAnimationPath" << HeavyAttackAnimationPath;
    Ar << "LightAttackSoundPath" << LightAttackSoundPath;
    Ar << "HeavyAttackSoundPath" << HeavyAttackSoundPath;
    Ar << "MoveSpeed" << MoveSpeed;
    Ar << "SprintSpeedMultiplier" << SprintSpeedMultiplier;
    Ar << "LookSensitivityDegrees" << LookSensitivityDegrees;
    Ar << "LocomotionBlendTime" << LocomotionBlendTime;
    Ar << "IntoRunDuration" << IntoRunDuration;
    Ar << "LightAttackDuration" << LightAttackDuration;
    Ar << "HeavyAttackDuration" << HeavyAttackDuration;
    Ar << "MoveStartSpeedThreshold" << MoveStartSpeedThreshold;
    Ar << "RotateToMovement" << bRotateToMovement;
    Ar << "AutoConfigureAnimation" << bAutoConfigureAnimation;

    if (Ar.IsLoading())
    {
        MoveSpeed = std::max(0.0f, MoveSpeed);
        SprintSpeedMultiplier = std::max(1.0f, SprintSpeedMultiplier);
        LookSensitivityDegrees = std::max(0.0f, LookSensitivityDegrees);
        LocomotionBlendTime = std::max(0.0f, LocomotionBlendTime);
        IntoRunDuration = std::max(0.0f, IntoRunDuration);
        LightAttackDuration = std::max(0.0f, LightAttackDuration);
        HeavyAttackDuration = std::max(0.0f, HeavyAttackDuration);
        MoveStartSpeedThreshold = std::max(0.0f, MoveStartSpeedThreshold);

        LoadConfiguredSkeletalMesh();
        if (bAutoConfigureAnimation)
        {
            ConfigureLocomotionStateMachine();
        }
    }
}

void AAnimTestPawn::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    APawn::GetEditableProperties(OutProps);
    OutProps.push_back({ "Skeletal Mesh", EPropertyType::String, &SkeletalMeshPath });
    OutProps.push_back({ "Idle Animation", EPropertyType::String, &IdleAnimationPath });
    OutProps.push_back({ "Into Run Animation", EPropertyType::String, &IntoRunAnimationPath });
    OutProps.push_back({ "Run Animation", EPropertyType::String, &RunAnimationPath });
    OutProps.push_back({ "Homeguard Animation", EPropertyType::String, &HomeguardAnimationPath });
    OutProps.push_back({ "Light Attack Animation", EPropertyType::String, &LightAttackAnimationPath });
    OutProps.push_back({ "Heavy Attack Animation", EPropertyType::String, &HeavyAttackAnimationPath });
    OutProps.push_back({ "Light Attack Sound", EPropertyType::String, &LightAttackSoundPath });
    OutProps.push_back({ "Heavy Attack Sound", EPropertyType::String, &HeavyAttackSoundPath });
    OutProps.push_back({ "Move Speed", EPropertyType::Float, &MoveSpeed, 0.0f, 100.0f, 0.1f });
    OutProps.push_back({ "Sprint Speed Multiplier", EPropertyType::Float, &SprintSpeedMultiplier, 1.0f, 10.0f, 0.05f });
    OutProps.push_back({ "Look Sensitivity", EPropertyType::Float, &LookSensitivityDegrees, 0.0f, 5.0f, 0.01f });
    OutProps.push_back({ "Locomotion Blend Time", EPropertyType::Float, &LocomotionBlendTime, 0.0f, 5.0f, 0.01f });
    OutProps.push_back({ "Into Run Duration", EPropertyType::Float, &IntoRunDuration, 0.0f, 5.0f, 0.01f });
    OutProps.push_back({ "Light Attack Duration", EPropertyType::Float, &LightAttackDuration, 0.0f, 5.0f, 0.01f });
    OutProps.push_back({ "Heavy Attack Duration", EPropertyType::Float, &HeavyAttackDuration, 0.0f, 5.0f, 0.01f });
    OutProps.push_back({ "Move Start Speed Threshold", EPropertyType::Float, &MoveStartSpeedThreshold, 0.0f, 100.0f, 0.01f });
    OutProps.push_back({ "Rotate Mesh To Movement", EPropertyType::Bool, &bRotateToMovement });
    OutProps.push_back({ "Auto Configure Animation", EPropertyType::Bool, &bAutoConfigureAnimation });
}

void AAnimTestPawn::PostEditProperty(const char* PropertyName)
{
    APawn::PostEditProperty(PropertyName);
    if (!PropertyName)
    {
        return;
    }

    if (std::strcmp(PropertyName, "Skeletal Mesh") == 0)
    {
        LoadConfiguredSkeletalMesh();
    }

    if (std::strcmp(PropertyName, "Idle Animation") == 0 ||
        std::strcmp(PropertyName, "Into Run Animation") == 0 ||
        std::strcmp(PropertyName, "Run Animation") == 0 ||
        std::strcmp(PropertyName, "Homeguard Animation") == 0 ||
        std::strcmp(PropertyName, "Light Attack Animation") == 0 ||
        std::strcmp(PropertyName, "Heavy Attack Animation") == 0 ||
        std::strcmp(PropertyName, "Locomotion Blend Time") == 0 ||
        std::strcmp(PropertyName, "Into Run Duration") == 0 ||
        std::strcmp(PropertyName, "Light Attack Duration") == 0 ||
        std::strcmp(PropertyName, "Heavy Attack Duration") == 0 ||
        std::strcmp(PropertyName, "Move Start Speed Threshold") == 0 ||
        std::strcmp(PropertyName, "Auto Configure Animation") == 0)
    {
        if (bAutoConfigureAnimation)
        {
            ConfigureLocomotionStateMachine();
        }
    }
}

void AAnimTestPawn::LoadConfiguredSkeletalMesh()
{
    if (!SkeletalMeshComp || SkeletalMeshPath.empty())
    {
        return;
    }

    SkeletalMeshComp->SetSkeletalMesh(FResourceManager::Get().LoadSkeletalMesh(SkeletalMeshPath));
}

void AAnimTestPawn::ConfigureLocomotionStateMachine()
{
    if (!SkeletalMeshComp ||
        IdleAnimationPath.empty() ||
        IntoRunAnimationPath.empty() ||
        RunAnimationPath.empty() ||
        HomeguardAnimationPath.empty() ||
        LightAttackAnimationPath.empty() ||
        HeavyAttackAnimationPath.empty())
    {
        return;
    }

    UAnimInstance* AnimInstance = SkeletalMeshComp->GetOrCreateDefaultAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    AnimInstance->RegisterAnimationPath(FName("Idle"), IdleAnimationPath);
    AnimInstance->RegisterAnimationPath(FName("IntoRun"), IntoRunAnimationPath);
    AnimInstance->RegisterAnimationPath(FName("Run"), RunAnimationPath);
    AnimInstance->RegisterAnimationPath(FName("Homeguard"), HomeguardAnimationPath);
    AnimInstance->RegisterAnimationPath(NAME_LightAttack, LightAttackAnimationPath);
    AnimInstance->RegisterAnimationPath(NAME_HeavyAttack, HeavyAttackAnimationPath);
    AddDefaultAnimationNotifies(AnimInstance);
    AnimInstance->SetAnimFloatParameter(FName("Speed"), 0.0f);
    AnimInstance->SetAnimFloatParameter(FName("GroundSpeed"), 0.0f);
    AnimInstance->SetAnimBoolParameter(FName("bMoving"), false);
    AnimInstance->SetAnimBoolParameter(FName("bSprinting"), false);

    if (LocomotionStateMachine)
    {
        UObjectManager::Get().DestroyObject(LocomotionStateMachine);
        LocomotionStateMachine = nullptr;
    }

    LocomotionStateMachine = UObjectManager::Get().CreateObject<UAnimStateMachineAsset>();
    if (!LocomotionStateMachine)
    {
        return;
    }

    LocomotionStateMachine->SetEntryState(FName("Idle"));
    LocomotionStateMachine->AddState(FName("Idle"), FName("Idle"), true, IdleAnimationPath);
    LocomotionStateMachine->AddState(FName("IntoRun"), FName("IntoRun"), false, IntoRunAnimationPath);
    LocomotionStateMachine->AddState(FName("Run"), FName("Run"), true, RunAnimationPath);
    LocomotionStateMachine->AddState(FName("Homeguard"), FName("Homeguard"), true, HomeguardAnimationPath);
    LocomotionStateMachine->AddState(FName("LightAttack"), FName("LightAttack"), false, LightAttackAnimationPath);
    LocomotionStateMachine->AddState(FName("HeavyAttack"), FName("HeavyAttack"), false, HeavyAttackAnimationPath);

    LocomotionStateMachine->AddBoolTransition(
        FName("Idle"),
        FName("Homeguard"),
        FName("bSprinting"),
        EAnimCompareOp::IsTrue,
        true,
        LocomotionBlendTime,
        20,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddBoolTransition(
        FName("Idle"),
        FName("IntoRun"),
        FName("bMoving"),
        EAnimCompareOp::IsTrue,
        true,
        LocomotionBlendTime,
        10,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddBoolTransition(
        FName("IntoRun"),
        FName("Homeguard"),
        FName("bSprinting"),
        EAnimCompareOp::IsTrue,
        true,
        LocomotionBlendTime,
        110,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddBoolTransition(
        FName("IntoRun"),
        FName("Idle"),
        FName("bMoving"),
        EAnimCompareOp::IsFalse,
        false,
        LocomotionBlendTime,
        100,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddFloatTransition(
        FName("IntoRun"),
        FName("Run"),
        FName("StateElapsedTime"),
        EAnimCompareOp::GreaterEqual,
        IntoRunDuration,
        LocomotionBlendTime,
        10,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddBoolTransition(
        FName("Run"),
        FName("Homeguard"),
        FName("bSprinting"),
        EAnimCompareOp::IsTrue,
        true,
        LocomotionBlendTime,
        110,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddBoolTransition(
        FName("Run"),
        FName("Idle"),
        FName("bMoving"),
        EAnimCompareOp::IsFalse,
        false,
        LocomotionBlendTime,
        100,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddBoolTransition(
        FName("Homeguard"),
        FName("Idle"),
        FName("bMoving"),
        EAnimCompareOp::IsFalse,
        false,
        LocomotionBlendTime,
        100,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddBoolTransition(
        FName("Homeguard"),
        FName("Run"),
        FName("bSprinting"),
        EAnimCompareOp::IsFalse,
        false,
        LocomotionBlendTime,
        90,
        EAnimBlendEaseOption::EaseInOut);

    const FName LocomotionStates[] = {
        FName("Idle"),
        FName("IntoRun"),
        FName("Run"),
        FName("Homeguard"),
    };

    for (const FName& StateName : LocomotionStates)
    {
        LocomotionStateMachine->AddTriggerTransition(
            StateName,
            FName("HeavyAttack"),
            FName("HeavyAttack"),
            LocomotionBlendTime,
            200,
            EAnimBlendEaseOption::EaseInOut);
        LocomotionStateMachine->AddTriggerTransition(
            StateName,
            FName("LightAttack"),
            FName("LightAttack"),
            LocomotionBlendTime,
            190,
            EAnimBlendEaseOption::EaseInOut);
    }

    TArray<FAnimTransitionCondition> LightAttackToIdleConditions;
    FAnimTransitionCondition LightAttackIdleTimeCondition;
    LightAttackIdleTimeCondition.ParameterName = FName("StateElapsedTime");
    LightAttackIdleTimeCondition.ParameterType = EAnimParameterType::Float;
    LightAttackIdleTimeCondition.CompareOp = EAnimCompareOp::GreaterEqual;
    LightAttackIdleTimeCondition.CompareValue.FloatValue = LightAttackDuration;
    LightAttackToIdleConditions.push_back(LightAttackIdleTimeCondition);
    FAnimTransitionCondition NotMovingCondition;
    NotMovingCondition.ParameterName = FName("bMoving");
    NotMovingCondition.ParameterType = EAnimParameterType::Bool;
    NotMovingCondition.CompareOp = EAnimCompareOp::IsFalse;
    LightAttackToIdleConditions.push_back(NotMovingCondition);
    LocomotionStateMachine->AddTransition(
        FName("LightAttack"),
        FName("Idle"),
        LightAttackToIdleConditions,
        LocomotionBlendTime,
        100,
        EAnimBlendEaseOption::EaseInOut);

    TArray<FAnimTransitionCondition> LightAttackToRunConditions;
    FAnimTransitionCondition LightAttackRunTimeCondition = LightAttackIdleTimeCondition;
    LightAttackToRunConditions.push_back(LightAttackRunTimeCondition);
    FAnimTransitionCondition MovingCondition;
    MovingCondition.ParameterName = FName("bMoving");
    MovingCondition.ParameterType = EAnimParameterType::Bool;
    MovingCondition.CompareOp = EAnimCompareOp::IsTrue;
    MovingCondition.CompareValue.BoolValue = true;

    FAnimTransitionCondition SprintingCondition;
    SprintingCondition.ParameterName = FName("bSprinting");
    SprintingCondition.ParameterType = EAnimParameterType::Bool;
    SprintingCondition.CompareOp = EAnimCompareOp::IsTrue;
    SprintingCondition.CompareValue.BoolValue = true;

    TArray<FAnimTransitionCondition> LightAttackToHomeguardConditions;
    FAnimTransitionCondition LightAttackHomeguardTimeCondition = LightAttackIdleTimeCondition;
    LightAttackToHomeguardConditions.push_back(LightAttackHomeguardTimeCondition);
    LightAttackToHomeguardConditions.push_back(SprintingCondition);
    LocomotionStateMachine->AddTransition(
        FName("LightAttack"),
        FName("Homeguard"),
        LightAttackToHomeguardConditions,
        LocomotionBlendTime,
        110,
        EAnimBlendEaseOption::EaseInOut);

    LightAttackToRunConditions.push_back(MovingCondition);
    LocomotionStateMachine->AddTransition(
        FName("LightAttack"),
        FName("Run"),
        LightAttackToRunConditions,
        LocomotionBlendTime,
        90,
        EAnimBlendEaseOption::EaseInOut);

    TArray<FAnimTransitionCondition> HeavyAttackToIdleConditions;
    FAnimTransitionCondition HeavyAttackIdleTimeCondition;
    HeavyAttackIdleTimeCondition.ParameterName = FName("StateElapsedTime");
    HeavyAttackIdleTimeCondition.ParameterType = EAnimParameterType::Float;
    HeavyAttackIdleTimeCondition.CompareOp = EAnimCompareOp::GreaterEqual;
    HeavyAttackIdleTimeCondition.CompareValue.FloatValue = HeavyAttackDuration;
    HeavyAttackToIdleConditions.push_back(HeavyAttackIdleTimeCondition);
    HeavyAttackToIdleConditions.push_back(NotMovingCondition);
    LocomotionStateMachine->AddTransition(
        FName("HeavyAttack"),
        FName("Idle"),
        HeavyAttackToIdleConditions,
        LocomotionBlendTime,
        100,
        EAnimBlendEaseOption::EaseInOut);

    TArray<FAnimTransitionCondition> HeavyAttackToRunConditions;
    FAnimTransitionCondition HeavyAttackRunTimeCondition = HeavyAttackIdleTimeCondition;
    HeavyAttackToRunConditions.push_back(HeavyAttackRunTimeCondition);

    TArray<FAnimTransitionCondition> HeavyAttackToHomeguardConditions;
    FAnimTransitionCondition HeavyAttackHomeguardTimeCondition = HeavyAttackIdleTimeCondition;
    HeavyAttackToHomeguardConditions.push_back(HeavyAttackHomeguardTimeCondition);
    HeavyAttackToHomeguardConditions.push_back(SprintingCondition);
    LocomotionStateMachine->AddTransition(
        FName("HeavyAttack"),
        FName("Homeguard"),
        HeavyAttackToHomeguardConditions,
        LocomotionBlendTime,
        110,
        EAnimBlendEaseOption::EaseInOut);

    HeavyAttackToRunConditions.push_back(MovingCondition);
    LocomotionStateMachine->AddTransition(
        FName("HeavyAttack"),
        FName("Run"),
        HeavyAttackToRunConditions,
        LocomotionBlendTime,
        90,
        EAnimBlendEaseOption::EaseInOut);

    if (!SkeletalMeshComp->UseStateMachine(LocomotionStateMachine))
    {
        UE_LOG_WARNING("[AnimTestPawn] Failed to activate locomotion state machine");
    }
}

void AAnimTestPawn::UpdateLocomotion(float DeltaTime)
{
    if (DeltaTime <= 0.0f)
    {
        UpdateAnimationParameters(0.0f, false, false);
        return;
    }

    APlayerController* PlayerController = GetController();
    const FGameplayInputSnapshot* Snapshot = PlayerController ? &PlayerController->GetInputSnapshot() : nullptr;
    const FInputActionState* MoveAction = Snapshot ? Snapshot->FindAction("Move") : nullptr;
    const FInputActionState* LookAction = Snapshot ? Snapshot->FindAction("Look") : nullptr;
    const FInputActionState* SprintAction = Snapshot ? Snapshot->FindAction("Sprint") : nullptr;
    const FInputActionState* LightAttackAction = Snapshot ? Snapshot->FindAction("Attack") : nullptr;
    const FInputActionState* HeavyAttackAction = Snapshot ? Snapshot->FindAction("HeavyAttack") : nullptr;
    const bool bLightAttackStarted = IsActionStarted(LightAttackAction);
    const bool bHeavyAttackStarted = IsActionStarted(HeavyAttackAction);
    const bool bLockMovement = IsInAttackState() || bLightAttackStarted || bHeavyAttackStarted;

    if (SpringArmComp && LookAction)
    {
        const FVector2& LookAxis = LookAction->Value.Axis2D;
        SpringArmComp->AddYawInput(LookAxis.X * LookSensitivityDegrees);
        SpringArmComp->AddPitchInput(-LookAxis.Y * LookSensitivityDegrees);
    }

    FVector2 MoveAxis = FVector2::ZeroVector;
    if (MoveAction)
    {
        MoveAxis = MoveAction->Value.Axis2D;
    }

    const float MoveAxisLength = std::min(AxisLength(MoveAxis), 1.0f);
    const bool bSprinting = MoveAxisLength > 0.0f && IsActionActive(SprintAction);
    const float GroundSpeed = MoveAxisLength * MoveSpeed * (bSprinting ? SprintSpeedMultiplier : 1.0f);
    const bool bMoving = GroundSpeed > MoveStartSpeedThreshold;

    if (bMoving && !bLockMovement)
    {
        const FVector MoveDirection = BuildCameraRelativeMoveDirection(MoveAxis);
        AddActorWorldOffset(MoveDirection * (GroundSpeed * DeltaTime));

        if (bRotateToMovement)
        {
            const float YawDegrees = MathUtil::RadiansToDegrees(std::atan2(MoveDirection.Y, MoveDirection.X)) - 90.0f;
            if (SkeletalMeshComp)
            {
                FVector MeshRotation = SkeletalMeshComp->GetRelativeRotation();
                MeshRotation.Z = FRotator::NormalizeAxis(YawDegrees - GetActorRotation().Z);
                SkeletalMeshComp->SetRelativeRotation(MeshRotation);
            }
        }
    }

    UpdateAnimationParameters(GroundSpeed, bMoving, bSprinting);

    if (SkeletalMeshComp)
    {
        if (UAnimInstance* AnimInstance = SkeletalMeshComp->GetAnimInstance())
        {
            if (bHeavyAttackStarted)
            {
                AnimInstance->SetAnimTriggerParameter(NAME_HeavyAttack);
            }
            else if (bLightAttackStarted)
            {
                AnimInstance->SetAnimTriggerParameter(NAME_LightAttack);
            }
        }
    }
}

void AAnimTestPawn::AddDefaultAnimationNotifies(UAnimInstance* AnimInstance)
{
    AddNotifyToAnimation(AnimInstance, NAME_LightAttack, NAME_LightAttackSwing, 0.18f);
    AddNotifyToAnimation(AnimInstance, NAME_HeavyAttack, NAME_HeavyAttackSwing, 0.28f);
}

void AAnimTestPawn::AddNotifyToAnimation(
    UAnimInstance* AnimInstance,
    const FName& AnimationName,
    const FName& NotifyName,
    float TriggerTime)
{
    if (!AnimInstance || !AnimationName.IsValid() || !NotifyName.IsValid())
    {
        return;
    }

    UAnimationSequenceBase* Sequence = AnimInstance->FindRegisteredAnimation(AnimationName);
    if (!Sequence)
    {
        return;
    }

    for (const FAnimNotifyEvent& ExistingNotify : Sequence->GetNotifies())
    {
        if (ExistingNotify.NotifyName == NotifyName)
        {
            return;
        }
    }

    FAnimNotifyEvent Notify;
    Notify.TriggerTime = std::max(0.0f, TriggerTime);
    Notify.Duration = 0.0f;
    Notify.NotifyName = NotifyName;
    Sequence->AddNotify(Notify);
}

bool AAnimTestPawn::IsInAttackState() const
{
    if (!SkeletalMeshComp)
    {
        return false;
    }

    const UAnimInstance* AnimInstance = SkeletalMeshComp->GetAnimInstance();
    if (!AnimInstance)
    {
        return false;
    }

    const FAnimStateMachineNode* StateMachine = AnimInstance->GetStateMachine();
    if (!StateMachine)
    {
        return false;
    }

    const FName CurrentState = StateMachine->GetCurrentState();
    return CurrentState == NAME_LightAttack || CurrentState == NAME_HeavyAttack;
}

void AAnimTestPawn::HandleAnimNotify(const FAnimNotifyEvent& Notify)
{
    APawn::HandleAnimNotify(Notify);
    PlayNotifySound(Notify);
}

void AAnimTestPawn::PlayNotifySound(const FAnimNotifyEvent& Notify)
{
    if (!GEngine || !Notify.NotifyName.IsValid())
    {
        return;
    }

    FString SoundPath;
    if (Notify.NotifyName == NAME_LightAttackSwing)
    {
        SoundPath = LightAttackSoundPath;
    }
    else if (Notify.NotifyName == NAME_HeavyAttackSwing)
    {
        SoundPath = HeavyAttackSoundPath;
    }

    if (!SoundPath.empty())
    {
        GEngine->GetAudioSystem().PlaySFX3D(SoundPath, GetActorLocation());
    }
}

FVector AAnimTestPawn::BuildCameraRelativeMoveDirection(const FVector2& MoveAxis) const
{
    FVector Forward = SpringArmComp ? SpringArmComp->GetForwardVector() : GetActorForward();
    FVector Right = SpringArmComp ? SpringArmComp->GetRightVector() : GetActorRight();

    Forward.Z = 0.0f;
    Right.Z = 0.0f;
    Forward.Normalize();
    Right.Normalize();

    FVector Direction = Forward * MoveAxis.Y + Right * MoveAxis.X;
    if (!Direction.Normalize())
    {
        return GetActorForward();
    }
    return Direction;
}

void AAnimTestPawn::UpdateAnimationParameters(float GroundSpeed, bool bMoving, bool bSprinting)
{
    if (!SkeletalMeshComp)
    {
        return;
    }

    UAnimInstance* AnimInstance = SkeletalMeshComp->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    AnimInstance->SetAnimFloatParameter(FName("Speed"), GroundSpeed);
    AnimInstance->SetAnimFloatParameter(FName("GroundSpeed"), GroundSpeed);
    AnimInstance->SetAnimBoolParameter(FName("bMoving"), bMoving);
    AnimInstance->SetAnimBoolParameter(FName("bSprinting"), bSprinting);
}
