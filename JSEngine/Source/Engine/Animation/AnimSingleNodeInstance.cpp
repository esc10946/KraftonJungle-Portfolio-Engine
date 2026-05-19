#include "Animation/AnimSingleNodeInstance.h"

void UAnimSingleNodeInstance::Initialize(USkeletalMeshComponent* InSkelMeshComponent)
{
    UAnimInstance::Initialize(InSkelMeshComponent);
    SequencePlayer.Initialize(this);
}

bool UAnimSingleNodeInstance::PlayAnimationByName(const FName& AnimationName, bool bLoop)
{
    return SequencePlayer.PlayAnimationByName(AnimationName, bLoop);
}

bool UAnimSingleNodeInstance::BlendToAnimationByName(
    const FName& AnimationName,
    bool bLoop,
    float BlendTime,
    EAnimBlendEaseOption EaseOption)
{
    return SequencePlayer.BlendToAnimationByName(AnimationName, bLoop, BlendTime, EaseOption);
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    SequencePlayer.Tick(DeltaSeconds);
}
