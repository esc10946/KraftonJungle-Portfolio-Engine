#include "SkeletalMeshComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimationStateMachine.h"
#include "Object/Object.h"
#include "Object/ObjectFactory.h"

DEFINE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)
REGISTER_FACTORY(USkeletalMeshComponent)

USkeletalMeshComponent::~USkeletalMeshComponent()
{
    if (AnimSingleNodeInstance)
    {
        if (AnimInstance == AnimSingleNodeInstance)
        {
            AnimInstance = nullptr;
        }

        UObjectManager::Get().DestroyObject(AnimSingleNodeInstance);
        AnimSingleNodeInstance = nullptr;
    }
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    USkinnedMeshComponent::TickComponent(DeltaTime);

    if (AnimInstance)
    {
        AnimInstance->TickAnimation(DeltaTime);
    }

	// Pose가 바뀐 경우에만 실제 CPU skinning이 수행(dirty flag 이용)
    EnsureSkinningUpdated();
}

void USkeletalMeshComponent::SetAnimInstance(UAnimInstance* InAnimInstance)
{
    if (AnimInstance == InAnimInstance)
    {
        return;
    }

    AnimInstance = InAnimInstance;
    if (AnimInstance)
    {
        AnimInstance->Initialize(this);
    }
}

bool USkeletalMeshComponent::UseStateMachine(UAnimStateMachineAsset* StateMachineAsset)
{
    if (!StateMachineAsset)
    {
        return false;
    }

    UAnimSingleNodeInstance* AnimSingleNode = GetOrCreateAnimSingleNodeInstance();
    if (!AnimSingleNode)
    {
        return false;
    }

    SetAnimInstance(AnimSingleNode);
    AnimSingleNode->SetStateMachineAsset(StateMachineAsset);
    return AnimSingleNode->GetStateMachineAsset() == StateMachineAsset;
}

bool USkeletalMeshComponent::LoadStateMachineFromJson(const FString& JsonPath)
{
    UAnimStateMachineAsset* StateMachineAsset = UAnimStateMachineAsset::LoadFromJsonFile(JsonPath);
    if (!StateMachineAsset)
    {
        return false;
    }

    return UseStateMachine(StateMachineAsset);
}

bool USkeletalMeshComponent::RegisterStateAnimationPath(const FName& AnimationName, const FString& AnimationPath)
{
    UAnimSingleNodeInstance* AnimSingleNode = GetOrCreateAnimSingleNodeInstance();
    if (!AnimSingleNode)
    {
        return false;
    }

    SetAnimInstance(AnimSingleNode);
    return AnimSingleNode->RegisterAnimationPath(AnimationName, AnimationPath);
}

void USkeletalMeshComponent::SetAnimStateMachineContext(const FAnimStateMachineContext& Context)
{
    UAnimSingleNodeInstance* AnimSingleNode = GetOrCreateAnimSingleNodeInstance();
    if (!AnimSingleNode)
    {
        return;
    }

    AnimSingleNode->SetStateMachineContext(Context);
}

UAnimSingleNodeInstance* USkeletalMeshComponent::GetOrCreateAnimSingleNodeInstance()
{
    if (!AnimSingleNodeInstance)
    {
        AnimSingleNodeInstance = UObjectManager::Get().CreateObject<UAnimSingleNodeInstance>();
        AnimSingleNodeInstance->SetLooping(true);
        AnimSingleNodeInstance->Initialize(this);
    }

    return AnimSingleNodeInstance;
}

void USkeletalMeshComponent::ApplyLocalPose(const TArray<FMatrix>& InLocalPose)
{
    if (InLocalPose.size() != CurrentLocalPose.size())
    {
        return;
    }

    CurrentLocalPose = InLocalPose;
    UpdateCurrentGlobalPose();
    MarkSkinningDirty();
}

void USkeletalMeshComponent::ResetToBindPose()
{
    InitializePoseFromBindPose();
    MarkSkinningDirty();
}

void USkeletalMeshComponent::SetBoneLocalTransform(int32 BoneIndex, const FMatrix& NewLocalTransform)
{
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(CurrentLocalPose.size()))
    {
        return;
    }

    CurrentLocalPose[BoneIndex] = NewLocalTransform;
    UpdateCurrentGlobalPose();
    MarkSkinningDirty();
}

const FMatrix& USkeletalMeshComponent::GetBoneLocalTransform(int32 BoneIndex) const
{
	// fallback은 identity
    static const FMatrix Identity = FMatrix::Identity;

    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(CurrentLocalPose.size()))
    {
        return Identity;
    }

    return CurrentLocalPose[BoneIndex];
}

FMatrix USkeletalMeshComponent::GetBoneGlobalTransform(int32 BoneIndex) const
{
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(CurrentGlobalPose.size()))
    {
        return FMatrix::Identity;
    }

    return CurrentGlobalPose[BoneIndex] * GetWorldMatrix();
}

void USkeletalMeshComponent::SetBoneGlobalTransform(int32 BoneIndex, const FMatrix& NewGlobalTransform)
{
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(CurrentLocalPose.size()) || !SkeletalMesh)
    {
        return;
    }

    const TArray<FBoneInfo>& Bones = SkeletalMesh->GetBones();
    if (BoneIndex >= static_cast<int32>(Bones.size()))
    {
        return;
    }

    int32 ParentIndex = Bones[BoneIndex].ParentIndex;

    FMatrix ParentGlobalTransform;
    if (ParentIndex >= 0)
    {
        ParentGlobalTransform = CurrentGlobalPose[ParentIndex] * GetWorldMatrix();
    }
    else
    {
        ParentGlobalTransform = GetWorldMatrix();
    }

    // Local = Global * ParentGlobal.Inverse
    FMatrix NewLocalTransform = NewGlobalTransform * ParentGlobalTransform.GetInverse();
    SetBoneLocalTransform(BoneIndex, NewLocalTransform);
}
