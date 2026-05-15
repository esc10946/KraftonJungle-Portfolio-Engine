#include "Animation/AnimInstance.h"

#include "Animation/AnimationStateMachine.h"
#include "Object/ObjectFactory.h"

#include <algorithm>
#include <cmath>

DEFINE_CLASS(UAnimInstance, UObject)
REGISTER_FACTORY(UAnimInstance)

DEFINE_CLASS(UAnimSingleNodeInstance, UAnimInstance)
REGISTER_FACTORY(UAnimSingleNodeInstance)

void UAnimInstance::Initialize(USkeletalMeshComponent* InSkelMeshComponent)
{
    SkelMeshComponent = InSkelMeshComponent;
}

void UAnimInstance::NativeUpdateAnimation(float)
{
}

void UAnimInstance::TickAnimation(float DeltaSeconds)
{
    if (StateMachine)
    {
        StateMachine->TickStateMachine(DeltaSeconds);
    }

    NativeUpdateAnimation(DeltaSeconds);
}

void UAnimSingleNodeInstance::SetAnimationAsset(UAnimationAssetBase* InAnimationAsset)
{
    AnimationAsset = InAnimationAsset;
    CurrentTime = 0.0f;
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
}

void UAnimSingleNodeInstance::SetCurrentTime(float InCurrentTime)
{
    const float PlayLength = AnimationAsset ? AnimationAsset->GetPlayLength() : 0.0f;
    CurrentTime = PlayLength > 0.0f
        ? std::clamp(InCurrentTime, 0.0f, PlayLength)
        : std::max(0.0f, InCurrentTime);
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    if (!AnimationAsset || !bPlaying || bPaused)
    {
        return;
    }

    const float PlayLength = AnimationAsset->GetPlayLength();
    CurrentTime += DeltaSeconds * PlayRate;

    if (PlayLength <= 0.0f)
    {
        CurrentTime = std::max(0.0f, CurrentTime);
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
}
