#pragma once

#include "Animation/AnimationAsset.h"
#include "Object/Object.h"

class USkeletalMeshComponent;
class UAnimationStateMachine;

class UAnimInstance : public UObject
{
public:
    DECLARE_CLASS(UAnimInstance, UObject)

    virtual void Initialize(USkeletalMeshComponent* InSkelMeshComponent);
    virtual void NativeUpdateAnimation(float DeltaSeconds);

    void TickAnimation(float DeltaSeconds);

    USkeletalMeshComponent* GetSkelMeshComponent() const { return SkelMeshComponent; }

    void SetStateMachine(UAnimationStateMachine* InStateMachine) { StateMachine = InStateMachine; }
    UAnimationStateMachine* GetStateMachine() const { return StateMachine; }

protected:
    USkeletalMeshComponent* SkelMeshComponent = nullptr;
    UAnimationStateMachine* StateMachine = nullptr;
};

class UAnimSingleNodeInstance : public UAnimInstance
{
public:
    DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

    void SetAnimationAsset(UAnimationAsset* InAnimationAsset);
    UAnimationAsset* GetAnimationAsset() const { return AnimationAsset; }

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

    void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
    UAnimationAsset* AnimationAsset = nullptr;
    float CurrentTime = 0.0f;
    float PlayRate = 1.0f;
    bool bPlaying = false;
    bool bPaused = false;
    bool bLooping = false;
};
