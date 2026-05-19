#include "Animation/StateMachineAnimInstance.h"

void UStateMachineAnimInstance::Initialize(USkeletalMeshComponent* InSkelMeshComponent)
{
    UAnimInstanceBase::Initialize(InSkelMeshComponent);
    SequencePlayer.Initialize(this);
}

void UStateMachineAnimInstance::TickAnimation(float DeltaSeconds)
{
    StateMachineNode.Update(DeltaSeconds, StateMachineContext);
    NativeUpdateAnimation(DeltaSeconds);
}

void UStateMachineAnimInstance::SetStateMachineAsset(UAnimStateMachineAsset* InStateMachineAsset)
{
    StateMachineNode.Initialize(InStateMachineAsset, this);
}

bool UStateMachineAnimInstance::PlayAnimationByName(const FName& AnimationName, bool bLoop)
{
    return SequencePlayer.PlayAnimationByName(AnimationName, bLoop);
}

bool UStateMachineAnimInstance::BlendToAnimationByName(
    const FName& AnimationName,
    bool bLoop,
    float BlendTime,
    EAnimBlendEaseOption EaseOption)
{
    return SequencePlayer.BlendToAnimationByName(AnimationName, bLoop, BlendTime, EaseOption);
}

void UStateMachineAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    SequencePlayer.Tick(DeltaSeconds);
}
