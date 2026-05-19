#pragma once

#include "Animation/AnimationSequenceBase.h"
#include "Animation/AnimationStateMachine.h"
#include "Object/Object.h"

class USkeletalMeshComponent;
class FAnimSequencePlayer;

#include "UAnimInstance.generated.h"

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
    bool RegisterAnimationPath(const FName& AnimationName, const FString& AnimationPath);
    UAnimationSequenceBase* FindRegisteredAnimation(const FName& AnimationName) const;
    virtual bool PlayAnimationByName(const FName& AnimationName, bool bLoop);
    virtual bool BlendToAnimationByName(
        const FName& AnimationName,
        bool bLoop,
        float BlendTime,
        EAnimBlendEaseOption EaseOption);

protected:
    friend class FAnimSequencePlayer;

    void TriggerAnimNotifies(
        UAnimationSequenceBase* AnimationSequence,
        float PreviousTime,
        float CurrentTime,
        float PlayLength,
        bool bLooped,
        bool bForwardPlayback);
    void TriggerAnimNotifiesInRange(
        UAnimationSequenceBase* AnimationSequence,
        float RangeStart,
        float RangeEnd);

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
