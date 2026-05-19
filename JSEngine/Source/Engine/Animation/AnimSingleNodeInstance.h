#pragma once

#include "Animation/AnimInstance.h"

#include "UAnimSingleNodeInstance.generated.h"

UCLASS()
class UAnimSingleNodeInstance : public UAnimInstance
{
public:
    GENERATED_BODY(UAnimSingleNodeInstance, UAnimInstance)

    void SetSequence(UAnimationSequenceBase* InSequence);
    UAnimationSequenceBase* GetSequence() const { return Sequence; }

    bool PlayAnimationByName(const FName& AnimationName, bool bLoop) override;
    bool BlendToAnimationByName(
        const FName& AnimationName,
        bool bLoop,
        float BlendTime,
        EAnimBlendEaseOption EaseOption) override;

    void Play();
    void Pause();
    void Stop();

    void SetLooping(bool bInLooping) { bLooping = bInLooping; }
    bool IsLooping() const { return bLooping; }

    void SetPlayRate(float InPlayRate) { PlayRate = InPlayRate; }
    float GetPlayRate() const { return PlayRate; }

    void SetCurrentTime(float InCurrentTime);
    float GetCurrentTime() const { return CurrentTime; }

    bool IsPlaying() const { return bPlaying; }
    bool IsPaused() const { return bPaused; }

    const TArray<FMatrix>& GetCurrentLocalPose() const { return CurrentLocalPose; }

    void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
    UAnimationSequenceBase* ResolveAnimationByName(const FName& AnimationName);
    bool AdvanceTime(float& InOutTime, float DeltaSeconds, float PlayLength, bool bInLooping, bool& bOutLooped);
    bool SampleCurrentPose();
    bool SamplePose(UAnimationSequenceBase* AnimationSequence, float Time, TArray<FMatrix>& OutLocalPose) const;
    bool SampleBlendedPose(float Alpha);
    float ApplyBlendEase(float Alpha) const;
    bool ApplyCurrentPoseToSkeletalMesh();
    void FinishBlend();

protected:
    UPROPERTY(Edit, LuaRead, LuaWrite)
    UAnimationSequenceBase* Sequence = nullptr;

    TArray<FMatrix> CurrentLocalPose;

    UPROPERTY(Edit, LuaRead, LuaWrite)
    float CurrentTime = 0.0f;

    UPROPERTY(Edit, LuaRead, LuaWrite)
    float PlayRate = 1.0f;

    UPROPERTY(Read, LuaRead)
    bool bPlaying = false;

    UPROPERTY(Read, LuaRead)
    bool bPaused = false;

    UPROPERTY(Edit, LuaRead, LuaWrite)
    bool bLooping = false;

    UAnimationSequenceBase* BlendSourceSequence = nullptr;
    UAnimationSequenceBase* BlendTargetSequence = nullptr;
    TArray<FMatrix> BlendSourcePose;
    TArray<FMatrix> BlendTargetPose;
    float BlendSourceTime = 0.0f;
    float BlendTargetTime = 0.0f;
    float BlendElapsedTime = 0.0f;
    float BlendDuration = 0.0f;
    EAnimBlendEaseOption BlendEaseOption = EAnimBlendEaseOption::Linear;
    bool bBlendSourceLooping = false;
    bool bBlending = false;
};
