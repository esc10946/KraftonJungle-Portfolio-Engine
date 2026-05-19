#include "Animation/AnimInstance.h"

#include "Component/SkeletalMeshComponent.h"
#include "Core/ResourceManager.h"

void UAnimInstance::Initialize(USkeletalMeshComponent* InSkelMeshComponent)
{
    SkelMeshComponent = InSkelMeshComponent;
}

void UAnimInstance::NativeUpdateAnimation(float)
{
}

void UAnimInstance::TickAnimation(float DeltaSeconds)
{
    NativeUpdateAnimation(DeltaSeconds);
}

void UAnimInstance::RegisterAnimation(const FName& AnimationName, UAnimationSequenceBase* Sequence)
{
    if (!AnimationName.IsValid())
    {
        return;
    }

    for (FNamedAnimation& RegisteredAnimation : RegisteredAnimations)
    {
        if (RegisteredAnimation.AnimationName == AnimationName)
        {
            RegisteredAnimation.Sequence = Sequence;
            return;
        }
    }

    FNamedAnimation RegisteredAnimation;
    RegisteredAnimation.AnimationName = AnimationName;
    RegisteredAnimation.Sequence = Sequence;
    RegisteredAnimations.push_back(RegisteredAnimation);
}

bool UAnimInstance::RegisterAnimationPath(const FName& AnimationName, const FString& AnimationPath)
{
    if (!AnimationName.IsValid() || AnimationPath.empty())
    {
        return false;
    }

    UAnimationSequenceBase* Sequence = FResourceManager::Get().LoadAnimationSequence(AnimationPath);
    if (!Sequence)
    {
        return false;
    }

    RegisterAnimation(AnimationName, Sequence);
    return true;
}

UAnimationSequenceBase* UAnimInstance::FindRegisteredAnimation(const FName& AnimationName) const
{
    for (const FNamedAnimation& RegisteredAnimation : RegisteredAnimations)
    {
        if (RegisteredAnimation.AnimationName == AnimationName)
        {
            return RegisteredAnimation.Sequence;
        }
    }

    return nullptr;
}

bool UAnimInstance::PlayAnimationByName(const FName&, bool)
{
    return false;
}

bool UAnimInstance::BlendToAnimationByName(
    const FName& AnimationName,
    bool bLoop,
    float,
    EAnimBlendEaseOption)
{
    return PlayAnimationByName(AnimationName, bLoop);
}

void UAnimInstance::TriggerAnimNotifies(
    UAnimationSequenceBase* AnimationSequence,
    float PreviousTime,
    float CurrentTime,
    float PlayLength,
    bool bLooped,
    bool bForwardPlayback)
{
    if (!AnimationSequence || !SkelMeshComponent || PlayLength <= 0.0f)
    {
        return;
    }

    if (bLooped)
    {
        if (bForwardPlayback)
        {
            TriggerAnimNotifiesInRange(AnimationSequence, PreviousTime, PlayLength);
            TriggerAnimNotifiesInRange(AnimationSequence, 0.0f, CurrentTime);
        }
        else
        {
            TriggerAnimNotifiesInRange(AnimationSequence, PreviousTime, 0.0f);
            TriggerAnimNotifiesInRange(AnimationSequence, PlayLength, CurrentTime);
        }
        return;
    }

    TriggerAnimNotifiesInRange(AnimationSequence, PreviousTime, CurrentTime);
}

void UAnimInstance::TriggerAnimNotifiesInRange(
    UAnimationSequenceBase* AnimationSequence,
    float RangeStart,
    float RangeEnd)
{
    if (!AnimationSequence || !SkelMeshComponent)
    {
        return;
    }

    const bool bForward = RangeStart <= RangeEnd;
    for (const FAnimNotifyEvent& Notify : AnimationSequence->GetNotifies())
    {
        if (!Notify.NotifyName.IsValid())
        {
            continue;
        }

        bool bShouldTrigger = false;
        if (bForward)
        {
            bShouldTrigger = Notify.TriggerTime > RangeStart && Notify.TriggerTime <= RangeEnd;
        }
        else
        {
            bShouldTrigger = Notify.TriggerTime < RangeStart && Notify.TriggerTime >= RangeEnd;
        }

        if (bShouldTrigger)
        {
            SkelMeshComponent->HandleAnimNotify(Notify);
        }
    }
}
