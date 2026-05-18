#pragma once

#include "Component/SkinnedMeshComponent.h"

#include "USkeletalMeshComponent.generated.h"
class UAnimStateMachineAsset;
class UAnimInstance;
class UAnimSingleNodeInstance;
struct FAnimStateMachineContext;

/**
 * @brief Unreal Engine 스타일에서는 skinned mesh가 skeleton을 이용하는 mesh를 표현하고,
 *        skeletal mesh는 실제로 actor에 붙어서 애니메이션을 붙일 수 있는 component로 사용되고 있으므로
 *        USkeletalMeshComponent 또한 해당 방식대로 우선은 얇게 유지.
 *        핵심 로직들은 대부분 USkinnedMeshComponent로 옮겼습니다.
 */
UCLASS()
class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
    GENERATED_BODY(USkeletalMeshComponent, USkinnedMeshComponent)
    USkeletalMeshComponent() = default;
    ~USkeletalMeshComponent() override;

    void TickComponent(float DeltaTime) override;

    EPrimitiveType GetPrimitiveType() const override { return EPrimitiveType::EPT_SkeletalMesh; }

    void SetAnimInstance(UAnimInstance* InAnimInstance);
    UAnimInstance* GetAnimInstance() const { return AnimInstance; }

    bool UseStateMachine(UAnimStateMachineAsset* StateMachineAsset);
    bool LoadStateMachineFromJson(const FString& JsonPath);
    bool RegisterStateAnimationPath(const FName& AnimationName, const FString& AnimationPath);
    void SetAnimStateMachineContext(const FAnimStateMachineContext& Context);

    void ApplyLocalPose(const TArray<FMatrix>& InLocalPose);

    void ResetToBindPose();

    void SetBoneLocalTransform(int32 BoneIndex, const FMatrix& NewLocalTransform);
    const FMatrix& GetBoneLocalTransform(int32 BoneIndex) const;

    FMatrix GetBoneGlobalTransform(int32 BoneIndex) const;
    void SetBoneGlobalTransform(int32 BoneIndex, const FMatrix& NewGlobalTransform);

private:
    UAnimSingleNodeInstance* GetOrCreateAnimSingleNodeInstance();

private:
    UAnimInstance* AnimInstance = nullptr;
    UAnimSingleNodeInstance* AnimSingleNodeInstance = nullptr;
};
