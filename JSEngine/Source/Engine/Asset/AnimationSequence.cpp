#include "Asset/AnimationSequence.h"

#include "Asset/SkeletalMesh.h"
#include "Object/ObjectFactory.h"

#include <algorithm>
#include <cmath>

DEFINE_CLASS(UAnimationSequence, UAnimationSequenceBase)
REGISTER_FACTORY(UAnimationSequence)

namespace
{
int32 GetNearestKeyIndex(float Time, const FAnimationSequence& SequenceData, int32 KeyCount)
{
    if (KeyCount <= 1)
    {
        return 0;
    }

    float ClampedTime = Time;
    if (ClampedTime < 0.0f)
    {
        ClampedTime = 0.0f;
    }

    if (SequenceData.DurationSeconds > 0.0f && ClampedTime > SequenceData.DurationSeconds)
    {
        ClampedTime = SequenceData.DurationSeconds;
    }

    int32 KeyIndex = 0;
    if (SequenceData.SourceFrameRate > 0.0f)
    {
        KeyIndex = static_cast<int32>(std::round(ClampedTime * SequenceData.SourceFrameRate));
    }
    else if (SequenceData.DurationSeconds > 0.0f)
    {
        const float NormalizedTime = ClampedTime / SequenceData.DurationSeconds;
        KeyIndex = static_cast<int32>(std::round(NormalizedTime * static_cast<float>(KeyCount - 1)));
    }

    return std::clamp(KeyIndex, 0, KeyCount - 1);
}

const FBoneAnimationTrack* FindBoneTrackByName(const FAnimationSequence& SequenceData, const FString& BoneName)
{
    for (const FBoneAnimationTrack& BoneTrack : SequenceData.BoneTracks)
    {
        if (BoneTrack.BoneName == BoneName)
        {
            return &BoneTrack;
        }
    }

    return nullptr;
}

FVector SampleTranslation(const FAnimationSequence& SequenceData, const FBoneAnimationTrack& BoneTrack, float Time)
{
    const TArray<FVector>& PosKeys = BoneTrack.InternalTrack.PosKeys;
    if (PosKeys.empty())
    {
        return BoneTrack.DefaultTranslation;
    }

    const int32 KeyIndex = GetNearestKeyIndex(Time, SequenceData, static_cast<int32>(PosKeys.size()));
    return PosKeys[KeyIndex];
}

FQuat SampleRotation(const FAnimationSequence& SequenceData, const FBoneAnimationTrack& BoneTrack, float Time)
{
    const TArray<FQuat>& RotKeys = BoneTrack.InternalTrack.RotKeys;
    if (RotKeys.empty())
    {
        return FQuat::MakeFromEuler(BoneTrack.DefaultRotationEuler);
    }

    const int32 KeyIndex = GetNearestKeyIndex(Time, SequenceData, static_cast<int32>(RotKeys.size()));
    return RotKeys[KeyIndex].GetNormalized();
}

FVector SampleScale(const FAnimationSequence& SequenceData, const FBoneAnimationTrack& BoneTrack, float Time)
{
    const TArray<FVector>& ScaleKeys = BoneTrack.InternalTrack.ScaleKeys;
    if (ScaleKeys.empty())
    {
        return BoneTrack.DefaultScale;
    }

    const int32 KeyIndex = GetNearestKeyIndex(Time, SequenceData, static_cast<int32>(ScaleKeys.size()));
    return ScaleKeys[KeyIndex];
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

    OutLocalPose.resize(TargetBones.size());
    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(TargetBones.size()); ++BoneIndex)
    {
        const FBoneInfo& TargetBone = TargetBones[BoneIndex];
        OutLocalPose[BoneIndex] = TargetBone.LocalBindTransform;

        const FBoneAnimationTrack* BoneTrack = FindBoneTrackByName(*SequenceData, TargetBone.Name);
        if (!BoneTrack)
        {
            continue;
        }

        const FVector Translation = SampleTranslation(*SequenceData, *BoneTrack, Time);
        const FQuat Rotation = SampleRotation(*SequenceData, *BoneTrack, Time);
        const FVector Scale = SampleScale(*SequenceData, *BoneTrack, Time);

        OutLocalPose[BoneIndex] = FMatrix::MakeTRS(Translation, Rotation.ToMatrix(), Scale);
    }

    return true;
}

