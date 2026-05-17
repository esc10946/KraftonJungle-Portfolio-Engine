#pragma once

#include "Animation/AnimationSequenceBase.h"
#include "Object/Object.h"

class USkeletalMeshComponent;
class UAnimationStateMachine;

#include "UAnimInstance.generated.h"
#include "UAnimSingleNodeInstance.generated.h"

UCLASS()
class UAnimInstance : public UObject
{
public:
    GENERATED_BODY(UAnimInstance, UObject)

    virtual void Initialize(USkeletalMeshComponent* InSkelMeshComponent);
    virtual void NativeUpdateAnimation(float DeltaSeconds);

    void TickAnimation(float DeltaSeconds);

    USkeletalMeshComponent* GetSkelMeshComponent() const { return SkelMeshComponent; }

    void SetStateMachine(UAnimationStateMachine* InStateMachine) { StateMachine = InStateMachine; }
    UAnimationStateMachine* GetStateMachine() const { return StateMachine; }

protected:
    UPROPERTY(Transient, Read)
    USkeletalMeshComponent* SkelMeshComponent = nullptr;

    UPROPERTY(Transient, Read, Write)
    UAnimationStateMachine* StateMachine = nullptr;
};

UCLASS()
class UAnimSingleNodeInstance : public UAnimInstance
{
public:
    GENERATED_BODY(UAnimSingleNodeInstance, UAnimInstance)

    void SetSequence(UAnimationSequenceBase* InSequence);
    UAnimationSequenceBase* GetSequence() const { return Sequence; }

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
    bool SampleCurrentPose();
    bool ApplyCurrentPoseToSkeletalMesh();

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
};
