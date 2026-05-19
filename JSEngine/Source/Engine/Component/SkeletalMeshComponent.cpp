#include "SkeletalMeshComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimationStateMachine.h"
#include "Animation/StateMachineAnimInstance.h"
#include "Component/Movement/MovementComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "Object/Object.h"
#include "Object/ObjectFactory.h"

DEFINE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)
REGISTER_FACTORY(USkeletalMeshComponent)

USkeletalMeshComponent::~USkeletalMeshComponent()
{
    if (StateMachineAnimInstance)
    {
        if (AnimInstance == StateMachineAnimInstance)
        {
            AnimInstance = nullptr;
        }

        UObjectManager::Get().DestroyObject(StateMachineAnimInstance);
        StateMachineAnimInstance = nullptr;
    }

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
        RefreshAnimStateMachineContextFromOwner();
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

    UStateMachineAnimInstance* StateMachineInstance = GetOrCreateStateMachineAnimInstance();
    if (!StateMachineInstance)
    {
        return false;
    }

    SetAnimInstance(StateMachineInstance);
    StateMachineInstance->SetStateMachineAsset(StateMachineAsset);
    return StateMachineInstance->GetStateMachineAsset() == StateMachineAsset;
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
    UStateMachineAnimInstance* StateMachineInstance = GetOrCreateStateMachineAnimInstance();
    if (!StateMachineInstance)
    {
        return false;
    }

    SetAnimInstance(StateMachineInstance);
    return StateMachineInstance->RegisterAnimationPath(AnimationName, AnimationPath);
}

void USkeletalMeshComponent::SetAnimStateMachineContext(const FAnimStateMachineContext& Context)
{
    UStateMachineAnimInstance* StateMachineInstance = GetOrCreateStateMachineAnimInstance();
    if (!StateMachineInstance)
    {
        return;
    }

    StateMachineInstance->SetStateMachineContext(Context);
}

void USkeletalMeshComponent::HandleAnimNotify(const FAnimNotifyEvent& Notify)
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        OwnerActor->HandleAnimNotify(Notify);
    }
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

UStateMachineAnimInstance* USkeletalMeshComponent::GetOrCreateStateMachineAnimInstance()
{
    if (!StateMachineAnimInstance)
    {
        StateMachineAnimInstance = UObjectManager::Get().CreateObject<UStateMachineAnimInstance>();
        StateMachineAnimInstance->SetLooping(true);
        StateMachineAnimInstance->Initialize(this);
    }

    return StateMachineAnimInstance;
}

void USkeletalMeshComponent::RefreshAnimStateMachineContextFromOwner()
{
    if (!bAutoUpdateAnimStateMachineContext || !StateMachineAnimInstance ||
        AnimInstance != StateMachineAnimInstance || !StateMachineAnimInstance->GetStateMachineAsset())
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    UMovementComponent* MovementComponent = OwnerActor->FindComponent<UMovementComponent>();
    if (!MovementComponent)
    {
        return;
    }

    FAnimStateMachineContext Context = StateMachineAnimInstance->GetStateMachineContext();
    const FVector Velocity = MovementComponent->GetVelocity();
    Context.Speed = Velocity.Size2D();

    const float MaxSpeed = MovementComponent->GetMaxSpeed();
    if (MaxSpeed > 0.0f)
    {
        Context.RunSpeed = MaxSpeed;
        if (Context.WalkSpeed <= 0.0f || Context.WalkSpeed > Context.RunSpeed)
        {
            Context.WalkSpeed = Context.RunSpeed * 0.5f;
        }
    }

    float FallingSpeedThreshold = Context.IdleSpeedThreshold;
    if (FallingSpeedThreshold <= 0.0f)
    {
        FallingSpeedThreshold = 1.0f;
    }

    if (Velocity.Z < -FallingSpeedThreshold)
    {
        Context.bIsGrounded = false;
        Context.MovementMode = EAnimStateMachineMovementMode::Falling;
    }
    else
    {
        Context.bIsGrounded = true;
        Context.MovementMode = EAnimStateMachineMovementMode::Walking;
    }

    StateMachineAnimInstance->SetStateMachineContext(Context);
}

void USkeletalMeshComponent::RegisterStateAnimationPathsFromAsset(const UAnimStateMachineAsset* StateMachineAsset)
{
    if (!StateMachineAsset)
    {
        return;
    }

    UStateMachineAnimInstance* StateMachineInstance = GetOrCreateStateMachineAnimInstance();
    if (!StateMachineInstance)
    {
        return;
    }

    for (const FAnimStateDesc& State : StateMachineAsset->GetStates())
    {
        if (!State.AnimationName.IsValid() || State.AnimationPath.empty())
        {
            continue;
        }

        if (!StateMachineInstance->RegisterAnimationPath(State.AnimationName, State.AnimationPath))
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
