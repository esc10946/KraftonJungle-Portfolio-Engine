#pragma once

#include "Asset/AnimationTypes.h"
#include "Animation/AnimationSequenceBase.h"

// FBX 등에서 import한 실제 animation sequence asset이다.
// FAnimationSequence 하나를 소유하고, 재생 계층에는 UAnimationSequenceBase API로 노출한다.
class UAnimationSequence : public UAnimationSequenceBase
{
public:
    DECLARE_CLASS(UAnimationSequence, UAnimationSequenceBase)

    UAnimationSequence() = default;
    ~UAnimationSequence() override;

    void SetSequenceData(FAnimationSequence* InSequenceData);

    FAnimationSequence* GetSequenceData();
    const FAnimationSequence* GetSequenceData() const;

    const FString& GetAssetPathFileName() const;
    const FString& GetSourcePath() const;

    bool HasValidSequenceData() const;

    float GetPlayLength() const override;
    bool SamplePose(const USkeletalMesh* TargetMesh, float Time, TArray<FMatrix>& OutLocalPose) const override;

private:
    FAnimationSequence* SequenceData = nullptr;
};

