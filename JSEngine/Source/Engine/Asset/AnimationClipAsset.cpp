#include "Asset/AnimationClipAsset.h"

DEFINE_CLASS(UAnimationClipAsset, UObject)

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
    static FString Empty = {};
    return ClipData ? ClipData->SourcePath : Empty;
}

bool UAnimationClipAsset::HasValidClipData() const
{
    return ClipData != nullptr && (!ClipData->BoneTracks.empty() || !ClipData->ShapeKeyTracks.empty());
}

