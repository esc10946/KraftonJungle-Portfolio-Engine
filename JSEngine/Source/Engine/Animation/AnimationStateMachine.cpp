#include "Animation/AnimationStateMachine.h"

#include "Object/ObjectFactory.h"

DEFINE_CLASS(UAnimationStateMachine, UObject)
REGISTER_FACTORY(UAnimationStateMachine)

void UAnimationStateMachine::Initialize(UAnimInstance* InAnimInstance)
{
    AnimInstance = InAnimInstance;
}

void UAnimationStateMachine::TickStateMachine(float DeltaSeconds)
{
    SetState(EvaluateState(DeltaSeconds));
}

EAnimState UAnimationStateMachine::EvaluateState(float)
{
    return CurrentState;
}

void UAnimationStateMachine::SetState(EAnimState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    PreviousState = CurrentState;
    CurrentState = NewState;
}

void UAnimationStateMachine::SetAnimationAssetForState(EAnimState State, UAnimationAssetBase* AnimationAsset)
{
    if (State == EAnimState::None)
    {
        return;
    }

    for (FAnimStateAnimation& StateAnimation : StateAnimations)
    {
        if (StateAnimation.State == State)
        {
            StateAnimation.AnimationAsset = AnimationAsset;
            return;
        }
    }

    FAnimStateAnimation NewStateAnimation;
    NewStateAnimation.State = State;
    NewStateAnimation.AnimationAsset = AnimationAsset;
    StateAnimations.push_back(NewStateAnimation);
}

UAnimationAssetBase* UAnimationStateMachine::GetAnimationAssetForState(EAnimState State) const
{
    for (const FAnimStateAnimation& StateAnimation : StateAnimations)
    {
        if (StateAnimation.State == State)
        {
            return StateAnimation.AnimationAsset;
        }
    }

    return nullptr;
}

UAnimationAssetBase* UAnimationStateMachine::GetCurrentAnimationAsset() const
{
    return GetAnimationAssetForState(CurrentState);
}
