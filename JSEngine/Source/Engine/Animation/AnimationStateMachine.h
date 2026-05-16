#pragma once

#include "Core/Containers/Array.h"
#include "Object/Object.h"

class UAnimInstance;
class UAnimationSequenceBase;

enum class EAnimState : uint8
{
    None = 0,
    Idle,
    Walk,
    Run,
    Fly,
};

struct FAnimStateAnimation
{
    EAnimState State = EAnimState::None;
    UAnimationSequenceBase* Sequence = nullptr;
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

    void SetSequenceForState(EAnimState State, UAnimationSequenceBase* Sequence);
    UAnimationSequenceBase* GetSequenceForState(EAnimState State) const;
    UAnimationSequenceBase* GetCurrentSequence() const;

protected:
    void SetState(EAnimState NewState);

protected:
    UAnimInstance* AnimInstance = nullptr;
    TArray<FAnimStateAnimation> StateAnimations;
    EAnimState CurrentState = EAnimState::None;
    EAnimState PreviousState = EAnimState::None;
};
