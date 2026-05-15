#pragma once

#include "Asset/AnimationTypes.h"
#include "Object/Object.h"

class UAnimationClipAsset : public UObject
{
public:
    DECLARE_CLASS(UAnimationClipAsset, UObject)

    UAnimationClipAsset() = default;
    ~UAnimationClipAsset() override;

    void SetClipData(FAnimationClip* InClipData);

    FAnimationClip* GetClipData();
    const FAnimationClip* GetClipData() const;

    const FString& GetAssetPathFileName() const;

    bool HasValidClipData() const;

private:
    FAnimationClip* ClipData = nullptr;
};

