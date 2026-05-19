#pragma once

#include "GameFramework/Pawn.h"
#include "Math/Vector2.h"

#include "AAnimTestPawn.generated.h"

class UAnimStateMachineAsset;
class UCameraComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USpringArmComponent;

UCLASS()
class AAnimTestPawn : public APawn
{
public:
    GENERATED_BODY(AAnimTestPawn, APawn)

    AAnimTestPawn() = default;
    ~AAnimTestPawn() override;

    void InitDefaultComponents() override;
    void BeginPlay() override;
    void Tick(float DeltaTime) override;
    void Serialize(FArchive& Ar) override;
    void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    void PostEditProperty(const char* PropertyName) override;

    USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComp; }
    UCameraComponent* GetCameraComponent() const { return CameraComp; }

private:
    void LoadConfiguredSkeletalMesh();
    void ConfigureLocomotionStateMachine();
    void UpdateLocomotion(float DeltaTime);
    FVector BuildCameraRelativeMoveDirection(const FVector2& MoveAxis) const;
    void UpdateAnimationParameters(float GroundSpeed, bool bMoving, bool bSprinting);

private:
    USceneComponent* SceneRoot = nullptr;
    USkeletalMeshComponent* SkeletalMeshComp = nullptr;
    USpringArmComponent* SpringArmComp = nullptr;
    UCameraComponent* CameraComp = nullptr;
    UAnimStateMachineAsset* LocomotionStateMachine = nullptr;

    FString SkeletalMeshPath = "Asset/SkeletalMesh/GwenFBX/gwen_skeletalmesh_Buffbone_Cstm_Healthbar.bin";
    FString IdleAnimationPath = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Idle.anm.bin";
    FString IntoRunAnimationPath = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Into_Run.bin";
    FString RunAnimationPath = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Run.anm.bin";
    FString HomeguardAnimationPath = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Gwen_Homeguard.anm.bin";
    FString LightAttackAnimationPath = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Attack1.bin";
    FString HeavyAttackAnimationPath = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Attack2.bin";

    float MoveSpeed = 7.0f;
    float SprintSpeedMultiplier = 1.5f;
    float LookSensitivityDegrees = 0.12f;
    float LocomotionBlendTime = 0.12f;
    float IntoRunDuration = 0.25f;
    float LightAttackDuration = 0.55f;
    float HeavyAttackDuration = 0.75f;
    float MoveStartSpeedThreshold = 0.1f;
    bool bRotateToMovement = true;
    bool bAutoConfigureAnimation = true;
};
