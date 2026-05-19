#include "Animation/AnimSingleNodeInstance.h"

#include "Component/SkeletalMeshComponent.h"
#include "Core/ResourceManager.h"
#include "Math/Quat.h"
#include "Math/Vector.h"

#include <algorithm>
#include <cmath>

namespace
{
FMatrix BlendBoneTransform(const FMatrix& A, const FMatrix& B, float Alpha)
{
    FVector TranslationA;
    FVector TranslationB;
    FVector ScaleA;
    FVector ScaleB;
    FMatrix RotationA;
    FMatrix RotationB;

    if (!A.Decompose(TranslationA, RotationA, ScaleA) ||
        !B.Decompose(TranslationB, RotationB, ScaleB))
    {
        return A * (1.0f - Alpha) + B * Alpha;
    }

    const FVector BlendedTranslation = FVector::Lerp(TranslationA, TranslationB, Alpha);
    const FVector BlendedScale = FVector::Lerp(ScaleA, ScaleB, Alpha);
    const FQuat BlendedRotation = FQuat::Slerp(FQuat(RotationA), FQuat(RotationB), Alpha);

    return FMatrix::MakeTRS(BlendedTranslation, BlendedRotation.ToMatrix(), BlendedScale);
}
} // namespace

void UAnimSingleNodeInstance::SetSequence(UAnimationSequenceBase* InSequence)
{
    Sequence = InSequence;
    CurrentTime = 0.0f;
    CurrentLocalPose.clear();
    BlendSourceSequence = nullptr;
    BlendTargetSequence = nullptr;
    BlendSourcePose.clear();
    BlendTargetPose.clear();
    BlendSourceTime = 0.0f;
    BlendTargetTime = 0.0f;
    BlendElapsedTime = 0.0f;
    BlendDuration = 0.0f;
    BlendEaseOption = EAnimBlendEaseOption::Linear;
    bBlendSourceLooping = false;
    bBlending = false;
}

bool UAnimSingleNodeInstance::PlayAnimationByName(const FName& AnimationName, bool bLoop)
{
    UAnimationSequenceBase* TargetSequence = ResolveAnimationByName(AnimationName);

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

bool UAnimSingleNodeInstance::BlendToAnimationByName(
    const FName& AnimationName,
    bool bLoop,
    float BlendTime,
    EAnimBlendEaseOption EaseOption)
{
    UAnimationSequenceBase* TargetSequence = ResolveAnimationByName(AnimationName);
    if (!TargetSequence)
    {
        return false;
    }

    if (BlendTime <= 0.0f || !Sequence || Sequence == TargetSequence)
    {
        return PlayAnimationByName(AnimationName, bLoop);
    }

    BlendSourceSequence = Sequence;
    BlendTargetSequence = TargetSequence;
    BlendSourceTime = CurrentTime;
    BlendTargetTime = 0.0f;
    BlendElapsedTime = 0.0f;
    BlendDuration = BlendTime;
    BlendEaseOption = EaseOption;
    bBlendSourceLooping = bLooping;
    bBlending = true;

    Sequence = TargetSequence;
    CurrentTime = 0.0f;
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
    FinishBlend();
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

    if (bBlending && BlendSourceSequence && BlendTargetSequence)
    {
        const float SourcePlayLength = BlendSourceSequence->GetPlayLength();
        const float TargetPlayLength = BlendTargetSequence->GetPlayLength();
        const float PreviousTargetTime = BlendTargetTime;
        const bool bForwardPlayback = DeltaSeconds * PlayRate >= 0.0f;
        bool bSourceLooped = false;
        bool bTargetLooped = false;

        if (SourcePlayLength <= 0.0f || TargetPlayLength <= 0.0f)
        {
            FinishBlend();
            return;
        }

        AdvanceTime(BlendSourceTime, DeltaSeconds, SourcePlayLength, bBlendSourceLooping, bSourceLooped);
        AdvanceTime(BlendTargetTime, DeltaSeconds, TargetPlayLength, bLooping, bTargetLooped);
        CurrentTime = BlendTargetTime;

        BlendElapsedTime += std::fabs(DeltaSeconds);
        const float RawAlpha = std::clamp(BlendElapsedTime / BlendDuration, 0.0f, 1.0f);
        const float Alpha = ApplyBlendEase(RawAlpha);
        if (SampleBlendedPose(Alpha))
        {
            ApplyCurrentPoseToSkeletalMesh();
        }

        TriggerAnimNotifies(BlendTargetSequence, PreviousTargetTime, BlendTargetTime, TargetPlayLength, bTargetLooped, bForwardPlayback);

        if (Alpha >= 1.0f)
        {
            FinishBlend();
        }
        return;
    }

    const float PlayLength = Sequence->GetPlayLength();
    const float PreviousTime = CurrentTime;
    const bool bForwardPlayback = DeltaSeconds * PlayRate >= 0.0f;
    bool bLooped = false;

    if (PlayLength <= 0.0f)
    {
        CurrentTime = 0.0f;
        return;
    }

    bPlaying = AdvanceTime(CurrentTime, DeltaSeconds, PlayLength, bLooping, bLooped);

    if (SampleCurrentPose())
    {
        ApplyCurrentPoseToSkeletalMesh();
    }

    TriggerAnimNotifies(Sequence, PreviousTime, CurrentTime, PlayLength, bLooped, bForwardPlayback);
}

UAnimationSequenceBase* UAnimSingleNodeInstance::ResolveAnimationByName(const FName& AnimationName)
{
    UAnimationSequenceBase* TargetSequence = FindRegisteredAnimation(AnimationName);
    if (!TargetSequence && AnimationName.IsValid())
    {
        TargetSequence = FResourceManager::Get().LoadAnimationSequence(AnimationName.ToString());
        if (TargetSequence)
        {
            RegisterAnimation(AnimationName, TargetSequence);
        }
    }

    return TargetSequence;
}

bool UAnimSingleNodeInstance::AdvanceTime(
    float& InOutTime,
    float DeltaSeconds,
    float PlayLength,
    bool bInLooping,
    bool& bOutLooped)
{
    bOutLooped = false;
    InOutTime += DeltaSeconds * PlayRate;

    if (InOutTime > PlayLength)
    {
        if (bInLooping)
        {
            InOutTime = std::fmod(InOutTime, PlayLength);
            bOutLooped = true;
            return true;
        }

        InOutTime = PlayLength;
        return false;
    }

    if (InOutTime < 0.0f)
    {
        if (bInLooping)
        {
            InOutTime = std::fmod(InOutTime, PlayLength) + PlayLength;
            bOutLooped = true;
            return true;
        }

        InOutTime = 0.0f;
        return false;
    }

    return true;
}

bool UAnimSingleNodeInstance::SampleCurrentPose()
{
    return SamplePose(Sequence, CurrentTime, CurrentLocalPose);
}

bool UAnimSingleNodeInstance::SamplePose(
    UAnimationSequenceBase* AnimationSequence,
    float Time,
    TArray<FMatrix>& OutLocalPose) const
{
    if (!AnimationSequence || !SkelMeshComponent || !SkelMeshComponent->GetSkeletalMesh())
    {
        OutLocalPose.clear();
        return false;
    }

    TArray<FMatrix> SampledPose;
    if (!AnimationSequence->SamplePose(SkelMeshComponent->GetSkeletalMesh(), Time, SampledPose))
    {
        return false;
    }

    OutLocalPose = SampledPose;

    return true;
}

bool UAnimSingleNodeInstance::SampleBlendedPose(float Alpha)
{
    if (!SamplePose(BlendSourceSequence, BlendSourceTime, BlendSourcePose) ||
        !SamplePose(BlendTargetSequence, BlendTargetTime, BlendTargetPose) ||
        BlendSourcePose.size() != BlendTargetPose.size())
    {
        return false;
    }

    CurrentLocalPose.clear();
    CurrentLocalPose.reserve(BlendTargetPose.size());
    for (size_t BoneIndex = 0; BoneIndex < BlendTargetPose.size(); ++BoneIndex)
    {
        CurrentLocalPose.push_back(BlendBoneTransform(BlendSourcePose[BoneIndex], BlendTargetPose[BoneIndex], Alpha));
    }

    return true;
}

float UAnimSingleNodeInstance::ApplyBlendEase(float Alpha) const
{
    Alpha = std::clamp(Alpha, 0.0f, 1.0f);

    switch (BlendEaseOption)
    {
    case EAnimBlendEaseOption::EaseIn:
        return Alpha * Alpha;
    case EAnimBlendEaseOption::EaseOut:
    {
        const float InverseAlpha = 1.0f - Alpha;
        return 1.0f - InverseAlpha * InverseAlpha;
    }
    case EAnimBlendEaseOption::EaseInOut:
        return Alpha * Alpha * (3.0f - 2.0f * Alpha);
    case EAnimBlendEaseOption::Linear:
    default:
        return Alpha;
    }
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

void UAnimSingleNodeInstance::FinishBlend()
{
    BlendSourceSequence = nullptr;
    BlendTargetSequence = nullptr;
    BlendSourcePose.clear();
    BlendTargetPose.clear();
    BlendSourceTime = 0.0f;
    BlendTargetTime = 0.0f;
    BlendElapsedTime = 0.0f;
    BlendDuration = 0.0f;
    BlendEaseOption = EAnimBlendEaseOption::Linear;
    bBlendSourceLooping = false;
    bBlending = false;
}
