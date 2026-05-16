#include "Animation/AnimInstance.h"

#include "Animation/AnimationStateMachine.h"
#include "Component/SkeletalMeshComponent.h"
#include "Core/Logging/Log.h"
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

void UAnimSingleNodeInstance::SetSequence(UAnimationSequenceBase* InSequence)
{
    Sequence = InSequence;
    CurrentTime = 0.0f;
    CurrentLocalPose.clear();
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

    static int32 DebugSampleCounter = 0;
    ++DebugSampleCounter;
    if (DebugSampleCounter % 60 == 0)
    {
        UE_LOG(
            "[AnimSingleNodeInstance] SampleCurrentPose | Time=%.3f | PoseBones=%zu",
            CurrentTime,
            CurrentLocalPose.size());
    }

    return true;
}

bool UAnimSingleNodeInstance::ApplyCurrentPoseToSkeletalMesh()
{
    if (!SkelMeshComponent || CurrentLocalPose.empty())
    {
        return false;
    }

    SkelMeshComponent->ApplyLocalPose(CurrentLocalPose);

    static int32 DebugApplyCounter = 0;
    ++DebugApplyCounter;
    if (DebugApplyCounter % 60 == 0)
    {
        UE_LOG(
            "[AnimSingleNodeInstance] ApplyCurrentPoseToSkeletalMesh | PoseBones=%zu",
            CurrentLocalPose.size());
    }

    return true;
}
