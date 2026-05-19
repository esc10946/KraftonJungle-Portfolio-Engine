#pragma once

#include "Animation/AnimationSequenceBase.h"
#include "Animation/AnimationStateMachine.h"
#include "Object/Object.h"

class USkeletalMeshComponent;
class FAnimSequencePlayer;

#include "UAnimInstanceBase.generated.h"

UCLASS()
class UAnimInstanceBase : public UObject
{
public:
    GENERATED_BODY(UAnimInstanceBase, UObject)

    virtual void Initialize(USkeletalMeshComponent* InSkelMeshComponent);
    virtual void NativeUpdateAnimation(float DeltaSeconds);

    virtual void TickAnimation(float DeltaSeconds);

    USkeletalMeshComponent* GetSkelMeshComponent() const { return SkelMeshComponent; }

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
};
