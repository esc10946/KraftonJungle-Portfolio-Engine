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

    FString SkeletalMeshPath = "Asset/SkeletalMesh/GwenFBX/Gwen_skeletalmesh_Buffbone_Cstm_Healthbar.bin";
    FString IdleAnimationPath = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Idle.anm.bin";
    FString RunAnimationPath = "Asset/SkeletalMesh/GwenFBX/gwen_anim_Skeleton_Run.anm.bin";

    float WalkSpeed = 4.0f;
    float SprintSpeedMultiplier = 1.75f;
    float LookSensitivityDegrees = 0.12f;
    float IdleRunBlendTime = 0.18f;
    float MoveStartSpeedThreshold = 0.1f;
    bool bRotateToMovement = true;
    bool bAutoConfigureAnimation = true;
};
