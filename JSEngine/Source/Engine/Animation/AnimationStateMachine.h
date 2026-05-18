#pragma once

#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/CoreTypes.h"
#include "Object/FName.h"
#include "Object/Object.h"

class UAnimInstance;

enum class EAnimStateMachineMovementMode : uint8
{
    None = 0,
    Walking,
    Falling,
    Flying,
};

struct FAnimStateMachineContext
{
    float Speed = 0.0f;
    bool bIsGrounded = true;
    EAnimStateMachineMovementMode MovementMode = EAnimStateMachineMovementMode::Walking;
    float WalkSpeed = 300.0f;
    float RunSpeed = 600.0f;
    float IdleSpeedThreshold = 1.0f;
};

struct FAnimStateDesc
{
    FName StateName;
    FName AnimationName;
    FString AnimationPath;
    bool bLoop = true;
};

struct FAnimTransitionDesc
{
    FName FromState;
    FName ToState;
    FName ConditionName;
    float BlendTime = 0.0f;
    int32 Priority = 0;
};

#include "UAnimStateMachineAsset.generated.h"

UCLASS()
class UAnimStateMachineAsset : public UObject
{
public:
    GENERATED_BODY(UAnimStateMachineAsset, UObject)

    void SetEntryState(const FName& StateName);
    bool AddState(
        const FName& StateName,
        const FName& AnimationName,
        bool bLoop,
        const FString& AnimationPath = "");
    bool SetStateAnimationPath(const FName& StateName, const FString& AnimationPath);
    bool AddTransition(
        const FName& FromState,
        const FName& ToState,
        const FName& ConditionName,
        float BlendTime,
        int32 Priority);

    const FName& GetEntryState() const { return EntryState; }
    const TArray<FAnimStateDesc>& GetStates() const { return States; }
    const TArray<FAnimTransitionDesc>& GetTransitions() const { return Transitions; }

    const FAnimStateDesc* FindState(const FName& StateName) const;
    TArray<const FAnimTransitionDesc*> GetTransitionsFrom(const FName& StateName) const;

    bool Validate(FString* OutMessage = nullptr) const;

    static UAnimStateMachineAsset* LoadFromJsonFile(const FString& Path);

private:
    bool HasDuplicateTransition(
        const FName& FromState,
        const FName& ToState,
        const FName& ConditionName) const;

private:
    FName EntryState;
    TArray<FAnimStateDesc> States;
    TArray<FAnimTransitionDesc> Transitions;
};

class FAnimStateMachineInstance
{
public:
    void Initialize(UAnimStateMachineAsset* InAsset, UAnimInstance* InAnimInstance);
    void Update(float DeltaSeconds, const FAnimStateMachineContext& Context);
    void Reset();

    FName GetCurrentState() const { return CurrentState; }
    FName GetPreviousState() const { return PreviousState; }
    float GetStateElapsedTime() const { return StateElapsedTime; }
    UAnimStateMachineAsset* GetAsset() const { return Asset; }

private:
    void ChangeState(const FName& NewState, float BlendTime);
    bool EvaluateCondition(const FName& ConditionName, const FAnimStateMachineContext& Context) const;

private:
    UAnimStateMachineAsset* Asset = nullptr;
    UAnimInstance* AnimInstance = nullptr;
    FName CurrentState;
    FName PreviousState;
    float StateElapsedTime = 0.0f;
};
