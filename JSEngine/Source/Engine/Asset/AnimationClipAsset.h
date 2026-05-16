#pragma once

#include "Asset/AnimationTypes.h"
#include "Animation/AnimationAssetBase.h"

// FBX 등에서 import한 실제 animation clip asset이다.
// FAnimationClip 하나를 소유하고, 재생 계층에는 UAnimationAssetBase API로 노출한다.
class UAnimationClipAsset : public UAnimationAssetBase
{
public:
    DECLARE_CLASS(UAnimationClipAsset, UAnimationAssetBase)

    UAnimationClipAsset() = default;
    ~UAnimationClipAsset() override;

    void SetClipData(FAnimationClip* InClipData);

    FAnimationClip* GetClipData();
    const FAnimationClip* GetClipData() const;

    const FString& GetAssetPathFileName() const;
    const FString& GetSourcePath() const;

    bool HasValidClipData() const;

    float GetPlayLength() const override;

private:
    FAnimationClip* ClipData = nullptr;
};

