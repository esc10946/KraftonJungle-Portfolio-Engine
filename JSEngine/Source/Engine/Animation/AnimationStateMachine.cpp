#include "Animation/AnimationStateMachine.h"

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

void UAnimationStateMachine::SetSequenceForState(EAnimState State, UAnimationSequenceBase* Sequence)
{
    if (State == EAnimState::None)
    {
        return;
    }

    for (FAnimStateAnimation& StateAnimation : StateAnimations)
    {
        if (StateAnimation.State == State)
        {
            StateAnimation.Sequence = Sequence;
            return;
        }
    }

    FAnimStateAnimation NewStateAnimation;
    NewStateAnimation.State = State;
    NewStateAnimation.Sequence = Sequence;
    StateAnimations.push_back(NewStateAnimation);
}

UAnimationSequenceBase* UAnimationStateMachine::GetSequenceForState(EAnimState State) const
{
    for (const FAnimStateAnimation& StateAnimation : StateAnimations)
    {
        if (StateAnimation.State == State)
        {
            return StateAnimation.Sequence;
        }
    }

    return nullptr;
}

UAnimationSequenceBase* UAnimationStateMachine::GetCurrentSequence() const
{
    return GetSequenceForState(CurrentState);
}
