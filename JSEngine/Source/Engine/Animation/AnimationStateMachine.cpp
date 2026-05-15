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
