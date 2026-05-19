#include "GameFramework/AnimTestPawn.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimStateMachineAsset.h"
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

#include <algorithm>
#include <cmath>
#include <cstring>

DEFINE_CLASS(AAnimTestPawn, APawn)
REGISTER_FACTORY(AAnimTestPawn)

namespace
{
    bool IsActionActive(const FInputActionState* Action)
    {
        return Action &&
            (Action->TriggerEvent == EInputTriggerEvent::Started ||
             Action->TriggerEvent == EInputTriggerEvent::Triggered);
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
    Ar << "RunAnimationPath" << RunAnimationPath;
    Ar << "WalkSpeed" << WalkSpeed;
    Ar << "SprintSpeedMultiplier" << SprintSpeedMultiplier;
    Ar << "LookSensitivityDegrees" << LookSensitivityDegrees;
    Ar << "IdleRunBlendTime" << IdleRunBlendTime;
    Ar << "MoveStartSpeedThreshold" << MoveStartSpeedThreshold;
    Ar << "RotateToMovement" << bRotateToMovement;
    Ar << "AutoConfigureAnimation" << bAutoConfigureAnimation;

    if (Ar.IsLoading())
    {
        WalkSpeed = std::max(0.0f, WalkSpeed);
        SprintSpeedMultiplier = std::max(1.0f, SprintSpeedMultiplier);
        LookSensitivityDegrees = std::max(0.0f, LookSensitivityDegrees);
        IdleRunBlendTime = std::max(0.0f, IdleRunBlendTime);
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
    OutProps.push_back({ "Run Animation", EPropertyType::String, &RunAnimationPath });
    OutProps.push_back({ "Walk Speed", EPropertyType::Float, &WalkSpeed, 0.0f, 100.0f, 0.1f });
    OutProps.push_back({ "Sprint Speed Multiplier", EPropertyType::Float, &SprintSpeedMultiplier, 1.0f, 10.0f, 0.05f });
    OutProps.push_back({ "Look Sensitivity", EPropertyType::Float, &LookSensitivityDegrees, 0.0f, 5.0f, 0.01f });
    OutProps.push_back({ "Idle Run Blend Time", EPropertyType::Float, &IdleRunBlendTime, 0.0f, 5.0f, 0.01f });
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
        std::strcmp(PropertyName, "Run Animation") == 0 ||
        std::strcmp(PropertyName, "Idle Run Blend Time") == 0 ||
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
    if (!SkeletalMeshComp || IdleAnimationPath.empty() || RunAnimationPath.empty())
    {
        return;
    }

    UAnimInstance* AnimInstance = SkeletalMeshComp->GetOrCreateDefaultAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    AnimInstance->RegisterAnimationPath(FName("Idle"), IdleAnimationPath);
    AnimInstance->RegisterAnimationPath(FName("Run"), RunAnimationPath);
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
    LocomotionStateMachine->AddState(FName("Run"), FName("Run"), true, RunAnimationPath);
    LocomotionStateMachine->AddFloatTransition(
        FName("Idle"),
        FName("Run"),
        FName("Speed"),
        EAnimCompareOp::Greater,
        MoveStartSpeedThreshold,
        IdleRunBlendTime,
        0,
        EAnimBlendEaseOption::EaseInOut);
    LocomotionStateMachine->AddFloatTransition(
        FName("Run"),
        FName("Idle"),
        FName("Speed"),
        EAnimCompareOp::LessEqual,
        MoveStartSpeedThreshold,
        IdleRunBlendTime,
        0,
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
    const FInputActionState* SprintAction = Snapshot ? Snapshot->FindAction("Dash") : nullptr;

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
    const bool bSprinting = IsActionActive(SprintAction);
    const float EffectiveSpeed = WalkSpeed * (bSprinting ? SprintSpeedMultiplier : 1.0f);
    const float GroundSpeed = MoveAxisLength * EffectiveSpeed;
    const bool bMoving = GroundSpeed > MoveStartSpeedThreshold;

    if (bMoving)
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
