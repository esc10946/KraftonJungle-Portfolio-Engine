#include "SkeletalMeshComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimStateMachineAsset.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "Object/Object.h"
#include "Object/ObjectFactory.h"

DEFINE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)
REGISTER_FACTORY(USkeletalMeshComponent)

USkeletalMeshComponent::~USkeletalMeshComponent()
{
    if (DefaultAnimInstance)
    {
        if (AnimInstance == DefaultAnimInstance)
        {
            AnimInstance = nullptr;
        }

        UObjectManager::Get().DestroyObject(DefaultAnimInstance);
        DefaultAnimInstance = nullptr;
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

    UAnimInstance* StateAnimInstance = GetOrCreateDefaultAnimInstance();
    if (!StateAnimInstance)
    {
        return false;
    }

    SetAnimInstance(StateAnimInstance);
    StateAnimInstance->SetStateMachineAsset(StateMachineAsset);
    return StateAnimInstance->GetStateMachineAsset() == StateMachineAsset;
}

bool USkeletalMeshComponent::LoadStateMachineFromJson(const FString& JsonPath)
{
    UAnimStateMachineAsset* StateMachineAsset = UAnimStateMachineAsset::LoadFromJsonFile(JsonPath);
    if (!StateMachineAsset)
    {
        return false;
    }

    RegisterStateAnimationPathsFromAsset(StateMachineAsset);
    return UseStateMachine(StateMachineAsset);
}

bool USkeletalMeshComponent::RegisterStateAnimationPath(const FName& AnimationName, const FString& AnimationPath)
{
    UAnimInstance* StateAnimInstance = GetOrCreateDefaultAnimInstance();
    if (!StateAnimInstance)
    {
        return false;
    }

    return StateAnimInstance->RegisterAnimationPath(AnimationName, AnimationPath);
}

void USkeletalMeshComponent::SetAnimBoolParameter(const FName& Name, bool Value)
{
    UAnimInstance* TargetAnimInstance = AnimInstance ? AnimInstance : GetOrCreateDefaultAnimInstance();
    if (TargetAnimInstance)
    {
        TargetAnimInstance->SetAnimBoolParameter(Name, Value);
    }
}

void USkeletalMeshComponent::SetAnimIntParameter(const FName& Name, int32 Value)
{
    UAnimInstance* TargetAnimInstance = AnimInstance ? AnimInstance : GetOrCreateDefaultAnimInstance();
    if (TargetAnimInstance)
    {
        TargetAnimInstance->SetAnimIntParameter(Name, Value);
    }
}

void USkeletalMeshComponent::SetAnimFloatParameter(const FName& Name, float Value)
{
    UAnimInstance* TargetAnimInstance = AnimInstance ? AnimInstance : GetOrCreateDefaultAnimInstance();
    if (TargetAnimInstance)
    {
        TargetAnimInstance->SetAnimFloatParameter(Name, Value);
    }
}

void USkeletalMeshComponent::SetAnimVectorParameter(const FName& Name, const FVector& Value)
{
    UAnimInstance* TargetAnimInstance = AnimInstance ? AnimInstance : GetOrCreateDefaultAnimInstance();
    if (TargetAnimInstance)
    {
        TargetAnimInstance->SetAnimVectorParameter(Name, Value);
    }
}

void USkeletalMeshComponent::SetAnimTriggerParameter(const FName& Name)
{
    UAnimInstance* TargetAnimInstance = AnimInstance ? AnimInstance : GetOrCreateDefaultAnimInstance();
    if (TargetAnimInstance)
    {
        TargetAnimInstance->SetAnimTriggerParameter(Name);
    }
}

void USkeletalMeshComponent::HandleAnimNotify(const FAnimNotifyEvent& Notify)
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        OwnerActor->HandleAnimNotify(Notify);
    }
}

UAnimInstance* USkeletalMeshComponent::GetOrCreateDefaultAnimInstance()
{
    if (!DefaultAnimInstance)
    {
        DefaultAnimInstance = UObjectManager::Get().CreateObject<UAnimInstance>();
        DefaultAnimInstance->SetLooping(true);
        DefaultAnimInstance->Initialize(this);
    }

    return DefaultAnimInstance;
}

void USkeletalMeshComponent::RegisterStateAnimationPathsFromAsset(const UAnimStateMachineAsset* StateMachineAsset)
{
    if (!StateMachineAsset)
    {
        return;
    }

    UAnimInstance* StateAnimInstance = GetOrCreateDefaultAnimInstance();
    if (!StateAnimInstance)
    {
        return;
    }

    for (const FAnimStateDesc& State : StateMachineAsset->GetStates())
    {
        if (!State.AnimationName.IsValid() || State.AnimationPath.empty())
        {
            continue;
        }

        if (!StateAnimInstance->RegisterAnimationPath(State.AnimationName, State.AnimationPath))
        {
            UE_LOG_WARNING(
                "[AnimSM] Failed to register state animation path: state=%s animation=%s path=%s",
                State.StateName.ToString().c_str(),
                State.AnimationName.ToString().c_str(),
                State.AnimationPath.c_str());
        }
    }
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
