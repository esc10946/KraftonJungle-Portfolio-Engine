#pragma once

#include "Object/Object.h"

class UAnimInstance;

enum class EAnimState : uint8
{
    None = 0,
    Idle,
    Walk,
    Run,
    Fly,
};

class UAnimationStateMachine : public UObject
{
public:
    DECLARE_CLASS(UAnimationStateMachine, UObject)

    virtual void Initialize(UAnimInstance* InAnimInstance);
    virtual void TickStateMachine(float DeltaSeconds);
    virtual EAnimState EvaluateState(float DeltaSeconds);

    EAnimState GetCurrentState() const { return CurrentState; }
    EAnimState GetPreviousState() const { return PreviousState; }

protected:
    void SetState(EAnimState NewState);

protected:
    UAnimInstance* AnimInstance = nullptr;
    EAnimState CurrentState = EAnimState::None;
    EAnimState PreviousState = EAnimState::None;
};
