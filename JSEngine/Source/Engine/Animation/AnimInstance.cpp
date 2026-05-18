#include "Animation/AnimInstance.h"

#include "Animation/AnimationStateMachine.h"
#include "Component/SkeletalMeshComponent.h"

#include <algorithm>
#include <cmath>

void UAnimInstance::Initialize(USkeletalMeshComponent* InSkelMeshComponent)
{
    SkelMeshComponent = InSkelMeshComponent;
}

void UAnimInstance::NativeUpdateAnimation(float)
{
}

void UAnimInstance::TickAnimation(float DeltaSeconds)
{
    StateMachineInstance.Update(DeltaSeconds, StateMachineContext);

    NativeUpdateAnimation(DeltaSeconds);
}

void UAnimInstance::SetStateMachineAsset(UAnimStateMachineAsset* InStateMachineAsset)
{
    StateMachineInstance.Initialize(InStateMachineAsset, this);
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

void UAnimSingleNodeInstance::SetSequence(UAnimationSequenceBase* InSequence)
{
    Sequence = InSequence;
    CurrentTime = 0.0f;
    CurrentLocalPose.clear();
}

bool UAnimSingleNodeInstance::PlayAnimationByName(const FName& AnimationName, bool bLoop)
{
    UAnimationSequenceBase* TargetSequence = FindRegisteredAnimation(AnimationName);
    if (!TargetSequence)
    {
        return false;
    }

    if (Sequence != TargetSequence)
    {
        SetSequence(TargetSequence);
    }

    SetLooping(bLoop);
    Play();
    return true;
}

void UAnimSingleNodeInstance::Play()
{
    bPlaying = true;
    bPaused = false;
}

void UAnimSingleNodeInstance::Pause()
{
    if (bPlaying)
    {
        bPaused = true;
    }
}

void UAnimSingleNodeInstance::Stop()
{
    bPlaying = false;
    bPaused = false;
    CurrentTime = 0.0f;
    CurrentLocalPose.clear();
}

void UAnimSingleNodeInstance::SetCurrentTime(float InCurrentTime)
{
    float PlayLength = 0.0f;

    if (Sequence)
    {
        PlayLength = Sequence->GetPlayLength();
    }

    if (PlayLength > 0.0f)
    {
        CurrentTime = std::clamp(InCurrentTime, 0.0f, PlayLength);
    }
    else
    {
        CurrentTime = 0.0f;
    }

    if (SampleCurrentPose())
    {
        ApplyCurrentPoseToSkeletalMesh();
    }
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    if (!Sequence || !bPlaying || bPaused)
    {
        return;
    }

    const float PlayLength = Sequence->GetPlayLength();
    CurrentTime += DeltaSeconds * PlayRate;

    if (PlayLength <= 0.0f)
    {
        CurrentTime = 0.0f;
        return;
    }

    if (CurrentTime > PlayLength)
    {
        if (bLooping)
        {
            CurrentTime = std::fmod(CurrentTime, PlayLength);
        }
        else
        {
            CurrentTime = PlayLength;
            bPlaying = false;
        }
    }
    else if (CurrentTime < 0.0f)
    {
        if (bLooping)
        {
            CurrentTime = std::fmod(CurrentTime, PlayLength) + PlayLength;
        }
        else
        {
            CurrentTime = 0.0f;
            bPlaying = false;
        }
    }

    if (SampleCurrentPose())
    {
        ApplyCurrentPoseToSkeletalMesh();
    }
}

bool UAnimSingleNodeInstance::SampleCurrentPose()
{
    if (!Sequence || !SkelMeshComponent || !SkelMeshComponent->GetSkeletalMesh())
    {
        CurrentLocalPose.clear();
        return false;
    }

    TArray<FMatrix> SampledPose;
    if (!Sequence->SamplePose(SkelMeshComponent->GetSkeletalMesh(), CurrentTime, SampledPose))
    {
        return false;
    }

    CurrentLocalPose = SampledPose;

    return true;
}

bool UAnimSingleNodeInstance::ApplyCurrentPoseToSkeletalMesh()
{
    if (!SkelMeshComponent || CurrentLocalPose.empty())
    {
        return false;
    }

    SkelMeshComponent->ApplyLocalPose(CurrentLocalPose);

    return true;
}
