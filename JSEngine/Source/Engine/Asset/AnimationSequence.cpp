#include "Asset/AnimationSequence.h"

#include "Asset/SkeletalMesh.h"
#include "Core/Logging/Log.h"
#include "Object/ObjectFactory.h"

#include <algorithm>
#include <cmath>

DEFINE_CLASS(UAnimationSequence, UAnimationSequenceBase)
REGISTER_FACTORY(UAnimationSequence)

namespace
{
struct FKeyFrameRange
{
    int32 PrevIndex = 0;
    int32 NextIndex = 0;
    float Alpha = 0.0f;
};

float ClampSequenceTime(float Time, const FAnimationSequence& SequenceData)
{
    if (Time < 0.0f)
    {
        return 0.0f;
    }

    if (SequenceData.DurationSeconds > 0.0f && Time > SequenceData.DurationSeconds)
    {
        return SequenceData.DurationSeconds;
    }

    return Time;
}

FKeyFrameRange GetKeyFrameRange(float Time, const FAnimationSequence& SequenceData, int32 KeyCount)
{
    FKeyFrameRange Range;
    if (KeyCount <= 1)
    {
        return Range;
    }

    const float ClampedTime = ClampSequenceTime(Time, SequenceData);
    float KeyPosition = 0.0f;

    if (SequenceData.SourceFrameRate > 0.0f)
    {
        KeyPosition = ClampedTime * SequenceData.SourceFrameRate;
    }
    else if (SequenceData.DurationSeconds > 0.0f)
    {
        const float NormalizedTime = ClampedTime / SequenceData.DurationSeconds;
        KeyPosition = NormalizedTime * static_cast<float>(KeyCount - 1);
    }

    const int32 PrevIndex = static_cast<int32>(std::floor(KeyPosition));
    const int32 NextIndex = PrevIndex + 1;

    Range.PrevIndex = std::clamp(PrevIndex, 0, KeyCount - 1);
    Range.NextIndex = std::clamp(NextIndex, 0, KeyCount - 1);
    Range.Alpha = KeyPosition - static_cast<float>(Range.PrevIndex);
    Range.Alpha = std::clamp(Range.Alpha, 0.0f, 1.0f);

    return Range;
}

int32 FindBoneTrackIndexByName(const FAnimationSequence& SequenceData, const FString& BoneName)
{
    for (int32 TrackIndex = 0; TrackIndex < static_cast<int32>(SequenceData.BoneTracks.size()); ++TrackIndex)
    {
        const FBoneAnimationTrack& BoneTrack = SequenceData.BoneTracks[TrackIndex];
        if (BoneTrack.BoneName == BoneName)
        {
            return TrackIndex;
        }
    }

    return -1;
}

FVector SampleTranslation(const FAnimationSequence& SequenceData, const FBoneAnimationTrack& BoneTrack, float Time)
{
    const TArray<FVector>& PosKeys = BoneTrack.InternalTrack.PosKeys;
    if (PosKeys.empty())
    {
        return BoneTrack.DefaultTranslation;
    }

    const FKeyFrameRange Range = GetKeyFrameRange(Time, SequenceData, static_cast<int32>(PosKeys.size()));
    return FVector::Lerp(PosKeys[Range.PrevIndex], PosKeys[Range.NextIndex], Range.Alpha);
}

FQuat SampleRotation(const FAnimationSequence& SequenceData, const FBoneAnimationTrack& BoneTrack, float Time)
{
    const TArray<FQuat>& RotKeys = BoneTrack.InternalTrack.RotKeys;
    if (RotKeys.empty())
    {
        return FQuat::MakeFromEuler(BoneTrack.DefaultRotationEuler);
    }

    const FKeyFrameRange Range = GetKeyFrameRange(Time, SequenceData, static_cast<int32>(RotKeys.size()));
    return FQuat::Slerp(RotKeys[Range.PrevIndex], RotKeys[Range.NextIndex], Range.Alpha);
}

FVector SampleScale(const FAnimationSequence& SequenceData, const FBoneAnimationTrack& BoneTrack, float Time)
{
    const TArray<FVector>& ScaleKeys = BoneTrack.InternalTrack.ScaleKeys;
    if (ScaleKeys.empty())
    {
        return BoneTrack.DefaultScale;
    }

    const FKeyFrameRange Range = GetKeyFrameRange(Time, SequenceData, static_cast<int32>(ScaleKeys.size()));
    return FVector::Lerp(ScaleKeys[Range.PrevIndex], ScaleKeys[Range.NextIndex], Range.Alpha);
}

bool HasAnimatedKeys(const FBoneAnimationTrack& BoneTrack)
{
    return BoneTrack.InternalTrack.PosKeys.size() > 1 ||
        BoneTrack.InternalTrack.RotKeys.size() > 1 ||
        BoneTrack.InternalTrack.ScaleKeys.size() > 1;
}

FVector GetFirstTranslationKey(const FBoneAnimationTrack& BoneTrack)
{
    if (!BoneTrack.InternalTrack.PosKeys.empty())
    {
        return BoneTrack.InternalTrack.PosKeys[0];
    }

    return BoneTrack.DefaultTranslation;
}

FQuat GetFirstRotationKey(const FBoneAnimationTrack& BoneTrack)
{
    if (!BoneTrack.InternalTrack.RotKeys.empty())
    {
        return BoneTrack.InternalTrack.RotKeys[0];
    }

    return FQuat::MakeFromEuler(BoneTrack.DefaultRotationEuler);
}

FVector GetFirstScaleKey(const FBoneAnimationTrack& BoneTrack)
{
    if (!BoneTrack.InternalTrack.ScaleKeys.empty())
    {
        return BoneTrack.InternalTrack.ScaleKeys[0];
    }

    return BoneTrack.DefaultScale;
}

FMatrix BuildTrackTransform(const FVector& Translation, const FQuat& Rotation, const FVector& Scale)
{
    return FMatrix::MakeTRS(Translation, Rotation.ToMatrix(), Scale);
}

FMatrix BuildFirstKeyTransform(const FBoneAnimationTrack& BoneTrack)
{
    return BuildTrackTransform(
        GetFirstTranslationKey(BoneTrack),
        GetFirstRotationKey(BoneTrack),
        GetFirstScaleKey(BoneTrack));
}

FMatrix BuildSampledKeyTransform(const FAnimationSequence& SequenceData, const FBoneAnimationTrack& BoneTrack, float Time)
{
    return BuildTrackTransform(
        SampleTranslation(SequenceData, BoneTrack, Time),
        SampleRotation(SequenceData, BoneTrack, Time),
        SampleScale(SequenceData, BoneTrack, Time));
}
}

UAnimationSequence::~UAnimationSequence()
{
    delete SequenceData;
    SequenceData = nullptr;
}

void UAnimationSequence::SetSequenceData(FAnimationSequence* InSequenceData)
{
    if (SequenceData == InSequenceData)
    {
        return;
    }

    delete SequenceData;
    SequenceData = InSequenceData;
    CachedTargetMesh = nullptr;
    CachedTrackIndexByBoneIndex.clear();
}

FAnimationSequence* UAnimationSequence::GetSequenceData()
{
    return SequenceData;
}

const FAnimationSequence* UAnimationSequence::GetSequenceData() const
{
    return SequenceData;
}

const FString& UAnimationSequence::GetAssetPathFileName() const
{
    return GetSourcePath();
}

const FString& UAnimationSequence::GetSourcePath() const
{
    static FString Empty = {};
    return SequenceData ? SequenceData->SourcePath : Empty;
}

bool UAnimationSequence::HasValidSequenceData() const
{
    return SequenceData != nullptr && (!SequenceData->BoneTracks.empty() || !SequenceData->ShapeKeyTracks.empty());
}

float UAnimationSequence::GetPlayLength() const
{
    return SequenceData ? SequenceData->DurationSeconds : 0.0f;
}

bool UAnimationSequence::SamplePose(const USkeletalMesh* TargetMesh, float Time, TArray<FMatrix>& OutLocalPose) const
{
    if (!SequenceData || !TargetMesh)
    {
        OutLocalPose.clear();
        return false;
    }

    const TArray<FBoneInfo>& TargetBones = TargetMesh->GetBones();
    if (TargetBones.empty())
    {
        OutLocalPose.clear();
        return false;
    }

    if (CachedTargetMesh != TargetMesh || CachedTrackIndexByBoneIndex.size() != TargetBones.size())
    {
        RebuildTrackCache(TargetMesh);
    }

    OutLocalPose.resize(TargetBones.size());
    int32 MatchedTrackCount = 0;
    int32 AppliedAnimatedTrackCount = 0;
    int32 ChangedLocalPoseCount = 0;

    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(TargetBones.size()); ++BoneIndex)
    {
        const FBoneInfo& TargetBone = TargetBones[BoneIndex];
        OutLocalPose[BoneIndex] = TargetBone.LocalBindTransform;

        const int32 TrackIndex = CachedTrackIndexByBoneIndex[BoneIndex];
        if (TrackIndex < 0)
        {
            continue;
        }

        const FBoneAnimationTrack& BoneTrack = SequenceData->BoneTracks[TrackIndex];
        ++MatchedTrackCount;

        if (!HasAnimatedKeys(BoneTrack))
        {
            continue;
        }

        const FMatrix FirstKeyTransform = BuildFirstKeyTransform(BoneTrack);
        const FMatrix CurrentKeyTransform = BuildSampledKeyTransform(*SequenceData, BoneTrack, Time);
        const FMatrix AnimationDelta = FirstKeyTransform.GetInverse() * CurrentKeyTransform;

        OutLocalPose[BoneIndex] = TargetBone.LocalBindTransform * AnimationDelta;
        ++AppliedAnimatedTrackCount;

        if (!OutLocalPose[BoneIndex].Equals(TargetBone.LocalBindTransform, 0.001f))
        {
            ++ChangedLocalPoseCount;
        }
    }

    static int32 DebugSampleCounter = 0;
    ++DebugSampleCounter;
    if (DebugSampleCounter % 60 == 0)
    {
        UE_LOG(
            "[AnimationSequence] SamplePose | Sequence=%s | Time=%.3f | Bones=%zu | MatchedTracks=%d | AppliedAnimatedTracks=%d | ChangedLocalPoses=%d",
            SequenceData->Name.c_str(),
            Time,
            TargetBones.size(),
            MatchedTrackCount,
            AppliedAnimatedTrackCount,
            ChangedLocalPoseCount);
    }

    return true;
}

void UAnimationSequence::RebuildTrackCache(const USkeletalMesh* TargetMesh) const
{
    CachedTargetMesh = TargetMesh;
    CachedTrackIndexByBoneIndex.clear();

    if (!SequenceData || !TargetMesh)
    {
        return;
    }

    const TArray<FBoneInfo>& TargetBones = TargetMesh->GetBones();
    CachedTrackIndexByBoneIndex.resize(TargetBones.size());

    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(TargetBones.size()); ++BoneIndex)
    {
        CachedTrackIndexByBoneIndex[BoneIndex] = FindBoneTrackIndexByName(*SequenceData, TargetBones[BoneIndex].Name);
    }
}

