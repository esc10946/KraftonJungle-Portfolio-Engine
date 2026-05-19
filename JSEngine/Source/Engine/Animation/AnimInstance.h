#pragma once

#include "Animation/AnimationSequenceBase.h"
#include "Animation/AnimStateMachineNode.h"
#include "Object/Object.h"

#include <memory>

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

    void SetStateMachineAsset(UAnimStateMachineAsset* InStateMachineAsset);
    UAnimStateMachineAsset* GetStateMachineAsset() const;
    FAnimStateMachineNode* GetStateMachine();
    const FAnimStateMachineNode* GetStateMachine() const;

    void SetAnimBoolParameter(const FName& Name, bool Value);
    void SetAnimIntParameter(const FName& Name, int32 Value);
    void SetAnimFloatParameter(const FName& Name, float Value);
    void SetAnimVectorParameter(const FName& Name, const FVector& Value);
    void SetAnimTriggerParameter(const FName& Name);
    FAnimStateMachineParameterStore& GetAnimParameters() { return AnimParameters; }
    const FAnimStateMachineParameterStore& GetAnimParameters() const { return AnimParameters; }

    void SetLooping(bool bInLooping);
    bool IsLooping() const;

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

    // Root animation node placeholder. Today it can be a state machine node,
    // but the base instance owns it through the generic AnimNode boundary.
    std::unique_ptr<FAnimNodeBase> RootNode;
    FAnimStateMachineParameterStore AnimParameters;

private:
    FAnimStateMachineNode* GetOrCreateStateMachineRootNode();
};
