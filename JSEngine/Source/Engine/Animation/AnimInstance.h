#pragma once

#include "Animation/AnimationSequenceBase.h"
#include "Animation/AnimationStateMachine.h"
#include "Object/Object.h"

class USkeletalMeshComponent;

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

    void SetStateMachineAsset(UAnimStateMachineAsset* InStateMachineAsset);
    UAnimStateMachineAsset* GetStateMachineAsset() const { return StateMachineInstance.GetAsset(); }
    FAnimStateMachineInstance& GetStateMachineInstance() { return StateMachineInstance; }
    const FAnimStateMachineInstance& GetStateMachineInstance() const { return StateMachineInstance; }

    void SetStateMachineContext(const FAnimStateMachineContext& InContext) { StateMachineContext = InContext; }
    const FAnimStateMachineContext& GetStateMachineContext() const { return StateMachineContext; }

    void RegisterAnimation(const FName& AnimationName, UAnimationSequenceBase* Sequence);
    UAnimationSequenceBase* FindRegisteredAnimation(const FName& AnimationName) const;
    virtual bool PlayAnimationByName(const FName& AnimationName, bool bLoop);

protected:
    UPROPERTY(Transient, Read)
    USkeletalMeshComponent* SkelMeshComponent = nullptr;

    struct FNamedAnimation
    {
        FName AnimationName;
        UAnimationSequenceBase* Sequence = nullptr;
    };

    TArray<FNamedAnimation> RegisteredAnimations;
    FAnimStateMachineContext StateMachineContext;
    FAnimStateMachineInstance StateMachineInstance;
};

UCLASS()
class UAnimSingleNodeInstance : public UAnimInstance
{
public:
    GENERATED_BODY(UAnimSingleNodeInstance, UAnimInstance)

    void SetSequence(UAnimationSequenceBase* InSequence);
    UAnimationSequenceBase* GetSequence() const { return Sequence; }

    bool PlayAnimationByName(const FName& AnimationName, bool bLoop) override;

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
