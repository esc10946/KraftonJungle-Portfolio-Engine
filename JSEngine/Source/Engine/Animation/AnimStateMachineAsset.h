#pragma once

#include "Animation/AnimStateMachineTypes.h"
#include "Object/Object.h"

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
        const TArray<FAnimTransitionCondition>& Conditions,
        float BlendTime,
        int32 Priority,
        EAnimBlendEaseOption EaseOption = EAnimBlendEaseOption::Linear);
    bool AddBoolTransition(
        const FName& FromState,
        const FName& ToState,
        const FName& ParameterName,
        EAnimCompareOp CompareOp,
        bool Value,
        float BlendTime,
        int32 Priority,
        EAnimBlendEaseOption EaseOption = EAnimBlendEaseOption::Linear);
    bool AddIntTransition(
        const FName& FromState,
        const FName& ToState,
        const FName& ParameterName,
        EAnimCompareOp CompareOp,
        int32 Value,
        float BlendTime,
        int32 Priority,
        EAnimBlendEaseOption EaseOption = EAnimBlendEaseOption::Linear);
    bool AddFloatTransition(
        const FName& FromState,
        const FName& ToState,
        const FName& ParameterName,
        EAnimCompareOp CompareOp,
        float Value,
        float BlendTime,
        int32 Priority,
        EAnimBlendEaseOption EaseOption = EAnimBlendEaseOption::Linear);
    bool AddVectorTransition(
        const FName& FromState,
        const FName& ToState,
        const FName& ParameterName,
        EAnimCompareOp CompareOp,
        const FVector& Value,
        float BlendTime,
        int32 Priority,
        EAnimBlendEaseOption EaseOption = EAnimBlendEaseOption::Linear);
    bool AddTriggerTransition(
        const FName& FromState,
        const FName& ToState,
        const FName& ParameterName,
        float BlendTime,
        int32 Priority,
        EAnimBlendEaseOption EaseOption = EAnimBlendEaseOption::Linear);

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
        const TArray<FAnimTransitionCondition>& Conditions) const;

private:
    FName EntryState;
    TArray<FAnimStateDesc> States;
    TArray<FAnimTransitionDesc> Transitions;
};
