#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequencePlayer.h"

#include "UStateMachineAnimInstance.generated.h"

UCLASS()
class UStateMachineAnimInstance : public UAnimInstance
{
public:
    GENERATED_BODY(UStateMachineAnimInstance, UAnimInstance)

    void Initialize(USkeletalMeshComponent* InSkelMeshComponent) override;

    void SetLooping(bool bInLooping) { SequencePlayer.SetLooping(bInLooping); }
    bool IsLooping() const { return SequencePlayer.IsLooping(); }

    bool PlayAnimationByName(const FName& AnimationName, bool bLoop) override;
    bool BlendToAnimationByName(
        const FName& AnimationName,
        bool bLoop,
        float BlendTime,
        EAnimBlendEaseOption EaseOption) override;

    void NativeUpdateAnimation(float DeltaSeconds) override;

private:
    FAnimSequencePlayer SequencePlayer;
};
