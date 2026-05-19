#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequencePlayer.h"

#include "UAnimSingleNodeInstance.generated.h"

UCLASS()
class UAnimSingleNodeInstance : public UAnimInstance
{
public:
    GENERATED_BODY(UAnimSingleNodeInstance, UAnimInstance)

    void Initialize(USkeletalMeshComponent* InSkelMeshComponent) override;

    void SetSequence(UAnimationSequenceBase* InSequence) { SequencePlayer.SetSequence(InSequence); }
    UAnimationSequenceBase* GetSequence() const { return SequencePlayer.GetSequence(); }

    bool PlayAnimationByName(const FName& AnimationName, bool bLoop) override;
    bool BlendToAnimationByName(
        const FName& AnimationName,
        bool bLoop,
        float BlendTime,
        EAnimBlendEaseOption EaseOption) override;

    void Play() { SequencePlayer.Play(); }
    void Pause() { SequencePlayer.Pause(); }
    void Stop() { SequencePlayer.Stop(); }

    void SetLooping(bool bInLooping) { SequencePlayer.SetLooping(bInLooping); }
    bool IsLooping() const { return SequencePlayer.IsLooping(); }

    void SetPlayRate(float InPlayRate) { SequencePlayer.SetPlayRate(InPlayRate); }
    float GetPlayRate() const { return SequencePlayer.GetPlayRate(); }

    void SetCurrentTime(float InCurrentTime) { SequencePlayer.SetCurrentTime(InCurrentTime); }
    float GetCurrentTime() const { return SequencePlayer.GetCurrentTime(); }

    bool IsPlaying() const { return SequencePlayer.IsPlaying(); }
    bool IsPaused() const { return SequencePlayer.IsPaused(); }

    const TArray<FMatrix>& GetCurrentLocalPose() const { return SequencePlayer.GetCurrentLocalPose(); }

    void NativeUpdateAnimation(float DeltaSeconds) override;

private:
    FAnimSequencePlayer SequencePlayer;
};
