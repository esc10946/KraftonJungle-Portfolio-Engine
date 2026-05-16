#include "Asset/AnimationClipAsset.h"

#include "Object/ObjectFactory.h"

DEFINE_CLASS(UAnimationClipAsset, UAnimationAssetBase)
REGISTER_FACTORY(UAnimationClipAsset)

UAnimationClipAsset::~UAnimationClipAsset()
{
    delete ClipData;
    ClipData = nullptr;
}

void UAnimationClipAsset::SetClipData(FAnimationClip* InClipData)
{
    if (ClipData == InClipData)
    {
        return;
    }

    delete ClipData;
    ClipData = InClipData;
}

FAnimationClip* UAnimationClipAsset::GetClipData()
{
    return ClipData;
}

const FAnimationClip* UAnimationClipAsset::GetClipData() const
{
    return ClipData;
}

const FString& UAnimationClipAsset::GetAssetPathFileName() const
{
    return GetSourcePath();
}

const FString& UAnimationClipAsset::GetSourcePath() const
{
    static FString Empty = {};
    return ClipData ? ClipData->SourcePath : Empty;
}

bool UAnimationClipAsset::HasValidClipData() const
{
    return ClipData != nullptr && (!ClipData->BoneTracks.empty() || !ClipData->ShapeKeyTracks.empty());
}

float UAnimationClipAsset::GetPlayLength() const
{
    return ClipData ? ClipData->DurationSeconds : 0.0f;
}

