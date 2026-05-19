#include "Animation/AnimStateMachineAsset.h"

#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Object/Object.h"
#include "SimpleJSON/json.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace
{
FString GetJsonString(json::JSON& Object, const char* Key, const FString& DefaultValue = "")
{
    if (!Object.hasKey(Key))
    {
        return DefaultValue;
    }

    return Object[Key].ToString();
}

bool GetJsonBool(json::JSON& Object, const char* Key, bool bDefaultValue)
{
    if (!Object.hasKey(Key))
    {
        return bDefaultValue;
    }

    return Object[Key].ToBool();
}

float GetJsonFloat(json::JSON& Object, const char* Key, float DefaultValue)
{
    if (!Object.hasKey(Key))
    {
        return DefaultValue;
    }

    return static_cast<float>(Object[Key].ToFloat());
}

int32 GetJsonInt(json::JSON& Object, const char* Key, int32 DefaultValue)
{
    if (!Object.hasKey(Key))
    {
        return DefaultValue;
    }

    return static_cast<int32>(Object[Key].ToInt());
}

EAnimBlendEaseOption GetJsonEaseOption(json::JSON& Object, const char* Key)
{
    const FString EaseOption = GetJsonString(Object, Key, "Linear");
    if (EaseOption == "EaseIn")
    {
        return EAnimBlendEaseOption::EaseIn;
    }

    if (EaseOption == "EaseOut")
    {
        return EAnimBlendEaseOption::EaseOut;
    }

    if (EaseOption == "EaseInOut")
    {
        return EAnimBlendEaseOption::EaseInOut;
    }

    return EAnimBlendEaseOption::Linear;
}

EAnimParameterType ParseParameterType(const FString& TypeName)
{
    if (TypeName == "Int")
    {
        return EAnimParameterType::Int;
    }

    if (TypeName == "Float")
    {
        return EAnimParameterType::Float;
    }

    if (TypeName == "Vector")
    {
        return EAnimParameterType::Vector;
    }

    if (TypeName == "Trigger")
    {
        return EAnimParameterType::Trigger;
    }

    return EAnimParameterType::Bool;
}

EAnimCompareOp ParseCompareOp(const FString& OpName)
{
    if (OpName == "NotEqual" || OpName == "!=")
    {
        return EAnimCompareOp::NotEqual;
    }

    if (OpName == "Greater" || OpName == ">")
    {
        return EAnimCompareOp::Greater;
    }

    if (OpName == "GreaterEqual" || OpName == ">=")
    {
        return EAnimCompareOp::GreaterEqual;
    }

    if (OpName == "Less" || OpName == "<")
    {
        return EAnimCompareOp::Less;
    }

    if (OpName == "LessEqual" || OpName == "<=")
    {
        return EAnimCompareOp::LessEqual;
    }

    if (OpName == "IsTrue")
    {
        return EAnimCompareOp::IsTrue;
    }

    if (OpName == "IsFalse")
    {
        return EAnimCompareOp::IsFalse;
    }

    return EAnimCompareOp::Equal;
}

bool AreConditionsEqual(
    const TArray<FAnimTransitionCondition>& A,
    const TArray<FAnimTransitionCondition>& B)
{
    if (A.size() != B.size())
    {
        return false;
    }

    for (size_t Index = 0; Index < A.size(); ++Index)
    {
        if (A[Index].ParameterName != B[Index].ParameterName ||
            A[Index].ParameterType != B[Index].ParameterType ||
            A[Index].CompareOp != B[Index].CompareOp ||
            A[Index].CompareValue.BoolValue != B[Index].CompareValue.BoolValue ||
            A[Index].CompareValue.IntValue != B[Index].CompareValue.IntValue ||
            A[Index].CompareValue.FloatValue != B[Index].CompareValue.FloatValue ||
            A[Index].CompareValue.VectorValue != B[Index].CompareValue.VectorValue)
        {
            return false;
        }
    }

    return true;
}

FAnimTransitionCondition ParseJsonCondition(json::JSON& ConditionNode)
{
    FAnimTransitionCondition Condition;
    Condition.ParameterName = FName(GetJsonString(ConditionNode, "parameter"));
    if (!Condition.ParameterName.IsValid())
    {
        Condition.ParameterName = FName(GetJsonString(ConditionNode, "name"));
    }

    Condition.ParameterType = ParseParameterType(GetJsonString(ConditionNode, "type", "Bool"));
    Condition.CompareOp = ParseCompareOp(GetJsonString(ConditionNode, "op", "Equal"));

    if (Condition.ParameterType == EAnimParameterType::Bool)
    {
        Condition.CompareValue.BoolValue = GetJsonBool(ConditionNode, "value", true);
    }
    else if (Condition.ParameterType == EAnimParameterType::Int)
    {
        Condition.CompareValue.IntValue = GetJsonInt(ConditionNode, "value", 0);
    }
    else if (Condition.ParameterType == EAnimParameterType::Float)
    {
        Condition.CompareValue.FloatValue = GetJsonFloat(ConditionNode, "value", 0.0f);
    }
    else if (Condition.ParameterType == EAnimParameterType::Vector)
    {
        Condition.CompareValue.VectorValue = FVector(
            GetJsonFloat(ConditionNode, "x", GetJsonFloat(ConditionNode, "valueX", 0.0f)),
            GetJsonFloat(ConditionNode, "y", GetJsonFloat(ConditionNode, "valueY", 0.0f)),
            GetJsonFloat(ConditionNode, "z", GetJsonFloat(ConditionNode, "valueZ", 0.0f)));
    }

    return Condition;
}
} // namespace

void UAnimStateMachineAsset::SetEntryState(const FName& StateName)
{
    EntryState = StateName;
}

bool UAnimStateMachineAsset::AddState(
    const FName& StateName,
    const FName& AnimationName,
    bool bLoop,
    const FString& AnimationPath)
{
    if (!StateName.IsValid())
    {
        UE_LOG_WARNING("[AnimSM] Invalid state: empty state name");
        return false;
    }

    if (FindState(StateName))
    {
        UE_LOG_WARNING("[AnimSM] Invalid state: duplicate state %s", StateName.ToString().c_str());
        return false;
    }

    FAnimStateDesc State;
    State.StateName = StateName;
    State.AnimationName = AnimationName;
    State.AnimationPath = FPaths::Normalize(AnimationPath);
    State.bLoop = bLoop;
    States.push_back(State);
    return true;
}

bool UAnimStateMachineAsset::SetStateAnimationPath(const FName& StateName, const FString& AnimationPath)
{
    if (!StateName.IsValid() || AnimationPath.empty())
    {
        return false;
    }

    for (FAnimStateDesc& State : States)
    {
        if (State.StateName == StateName)
        {
            State.AnimationPath = FPaths::Normalize(AnimationPath);
            return true;
        }
    }

    UE_LOG_WARNING("[AnimSM] Failed to set animation path: missing state %s", StateName.ToString().c_str());
    return false;
}

bool UAnimStateMachineAsset::AddTransition(
    const FName& FromState,
    const FName& ToState,
    const TArray<FAnimTransitionCondition>& Conditions,
    float BlendTime,
    int32 Priority,
    EAnimBlendEaseOption EaseOption)
{
    if (!FromState.IsValid() || !ToState.IsValid() || Conditions.empty())
    {
        UE_LOG_WARNING("[AnimSM] Invalid transition: empty from/to/conditions");
        return false;
    }

    for (const FAnimTransitionCondition& Condition : Conditions)
    {
        if (!Condition.ParameterName.IsValid())
        {
            UE_LOG_WARNING("[AnimSM] Invalid transition: empty condition parameter");
            return false;
        }
    }

    if (HasDuplicateTransition(FromState, ToState, Conditions))
    {
        UE_LOG_WARNING(
            "[AnimSM] Invalid transition: duplicate %s -> %s",
            FromState.ToString().c_str(),
            ToState.ToString().c_str());
        return false;
    }

    FAnimTransitionDesc Transition;
    Transition.FromState = FromState;
    Transition.ToState = ToState;
    Transition.Conditions = Conditions;
    Transition.BlendTime = std::max(0.0f, BlendTime);
    Transition.EaseOption = EaseOption;
    Transition.Priority = Priority;
    Transitions.push_back(Transition);
    return true;
}

bool UAnimStateMachineAsset::AddBoolTransition(
    const FName& FromState,
    const FName& ToState,
    const FName& ParameterName,
    EAnimCompareOp CompareOp,
    bool Value,
    float BlendTime,
    int32 Priority,
    EAnimBlendEaseOption EaseOption)
{
    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
    Condition.ParameterType = EAnimParameterType::Bool;
    Condition.CompareOp = CompareOp;
    Condition.CompareValue.BoolValue = Value;

    TArray<FAnimTransitionCondition> Conditions;
    Conditions.push_back(Condition);
    return AddTransition(FromState, ToState, Conditions, BlendTime, Priority, EaseOption);
}

bool UAnimStateMachineAsset::AddIntTransition(
    const FName& FromState,
    const FName& ToState,
    const FName& ParameterName,
    EAnimCompareOp CompareOp,
    int32 Value,
    float BlendTime,
    int32 Priority,
    EAnimBlendEaseOption EaseOption)
{
    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
    Condition.ParameterType = EAnimParameterType::Int;
    Condition.CompareOp = CompareOp;
    Condition.CompareValue.IntValue = Value;

    TArray<FAnimTransitionCondition> Conditions;
    Conditions.push_back(Condition);
    return AddTransition(FromState, ToState, Conditions, BlendTime, Priority, EaseOption);
}

bool UAnimStateMachineAsset::AddFloatTransition(
    const FName& FromState,
    const FName& ToState,
    const FName& ParameterName,
    EAnimCompareOp CompareOp,
    float Value,
    float BlendTime,
    int32 Priority,
    EAnimBlendEaseOption EaseOption)
{
    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
    Condition.ParameterType = EAnimParameterType::Float;
    Condition.CompareOp = CompareOp;
    Condition.CompareValue.FloatValue = Value;

    TArray<FAnimTransitionCondition> Conditions;
    Conditions.push_back(Condition);
    return AddTransition(FromState, ToState, Conditions, BlendTime, Priority, EaseOption);
}

bool UAnimStateMachineAsset::AddVectorTransition(
    const FName& FromState,
    const FName& ToState,
    const FName& ParameterName,
    EAnimCompareOp CompareOp,
    const FVector& Value,
    float BlendTime,
    int32 Priority,
    EAnimBlendEaseOption EaseOption)
{
    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
    Condition.ParameterType = EAnimParameterType::Vector;
    Condition.CompareOp = CompareOp;
    Condition.CompareValue.VectorValue = Value;

    TArray<FAnimTransitionCondition> Conditions;
    Conditions.push_back(Condition);
    return AddTransition(FromState, ToState, Conditions, BlendTime, Priority, EaseOption);
}

bool UAnimStateMachineAsset::AddTriggerTransition(
    const FName& FromState,
    const FName& ToState,
    const FName& ParameterName,
    float BlendTime,
    int32 Priority,
    EAnimBlendEaseOption EaseOption)
{
    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
    Condition.ParameterType = EAnimParameterType::Trigger;
    Condition.CompareOp = EAnimCompareOp::IsTrue;

    TArray<FAnimTransitionCondition> Conditions;
    Conditions.push_back(Condition);
    return AddTransition(FromState, ToState, Conditions, BlendTime, Priority, EaseOption);
}

const FAnimStateDesc* UAnimStateMachineAsset::FindState(const FName& StateName) const
{
    for (const FAnimStateDesc& State : States)
    {
        if (State.StateName == StateName)
        {
            return &State;
        }
    }

    return nullptr;
}

TArray<const FAnimTransitionDesc*> UAnimStateMachineAsset::GetTransitionsFrom(const FName& StateName) const
{
    TArray<const FAnimTransitionDesc*> Result;
    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        if (Transition.FromState == StateName)
        {
            Result.push_back(&Transition);
        }
    }

    std::stable_sort(
        Result.begin(),
        Result.end(),
        [](const FAnimTransitionDesc* A, const FAnimTransitionDesc* B)
        {
            return A->Priority > B->Priority;
        });
    return Result;
}

bool UAnimStateMachineAsset::Validate(FString* OutMessage) const
{
    auto SetMessage = [OutMessage](const FString& Message)
    {
        if (OutMessage)
        {
            *OutMessage = Message;
        }
    };

    if (!EntryState.IsValid() || !FindState(EntryState))
    {
        SetMessage("Entry state is missing or does not exist");
        UE_LOG_WARNING("[AnimSM] Invalid asset: entry state is missing or does not exist");
        return false;
    }

    for (size_t StateIndex = 0; StateIndex < States.size(); ++StateIndex)
    {
        if (!States[StateIndex].StateName.IsValid())
        {
            SetMessage("State name is empty");
            UE_LOG_WARNING("[AnimSM] Invalid asset: state name is empty");
            return false;
        }

        for (size_t OtherIndex = StateIndex + 1; OtherIndex < States.size(); ++OtherIndex)
        {
            if (States[StateIndex].StateName == States[OtherIndex].StateName)
            {
                SetMessage("Duplicate state name");
                UE_LOG_WARNING("[AnimSM] Invalid asset: duplicate state %s", States[StateIndex].StateName.ToString().c_str());
                return false;
            }
        }
    }

    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        if (!FindState(Transition.FromState))
        {
            SetMessage("Transition from-state does not exist");
            UE_LOG_WARNING("[AnimSM] Invalid asset: from-state does not exist %s", Transition.FromState.ToString().c_str());
            return false;
        }

        if (!FindState(Transition.ToState))
        {
            SetMessage("Transition to-state does not exist");
            UE_LOG_WARNING("[AnimSM] Invalid asset: to-state does not exist %s", Transition.ToState.ToString().c_str());
            return false;
        }

        if (Transition.Conditions.empty())
        {
            SetMessage("Transition conditions are empty");
            UE_LOG_WARNING("[AnimSM] Invalid asset: transition conditions are empty");
            return false;
        }

        for (const FAnimTransitionCondition& Condition : Transition.Conditions)
        {
            if (!Condition.ParameterName.IsValid())
            {
                SetMessage("Transition condition parameter name is empty");
                UE_LOG_WARNING("[AnimSM] Invalid asset: transition condition parameter name is empty");
                return false;
            }
        }
    }

    if (OutMessage)
    {
        OutMessage->clear();
    }
    return true;
}

UAnimStateMachineAsset* UAnimStateMachineAsset::LoadFromJsonFile(const FString& Path)
{
    const FString NormalizedPath = FPaths::Normalize(Path);
    if (NormalizedPath.empty())
    {
        return nullptr;
    }

    std::ifstream File(FPaths::ToWide(NormalizedPath));
    if (!File.is_open())
    {
        UE_LOG_ERROR("[AnimSM] Failed to open state machine asset: %s", NormalizedPath.c_str());
        return nullptr;
    }

    FString FileContent((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
    json::JSON Root = json::JSON::Load(FileContent);
    if (Root.JSONType() != json::JSON::Class::Object)
    {
        UE_LOG_ERROR("[AnimSM] Invalid state machine json: %s", NormalizedPath.c_str());
        return nullptr;
    }

    UAnimStateMachineAsset* Asset = UObjectManager::Get().CreateObject<UAnimStateMachineAsset>();
    if (Root.hasKey("name"))
    {
        Asset->SetFName(FName(GetJsonString(Root, "name")));
    }

    Asset->SetEntryState(FName(GetJsonString(Root, "entryState")));

    if (Root.hasKey("states"))
    {
        for (json::JSON& StateNode : Root["states"].ArrayRange())
        {
            Asset->AddState(
                FName(GetJsonString(StateNode, "name")),
                FName(GetJsonString(StateNode, "animation")),
                GetJsonBool(StateNode, "loop", true),
                GetJsonString(StateNode, "animationPath"));
        }
    }

    if (Root.hasKey("transitions"))
    {
        for (json::JSON& TransitionNode : Root["transitions"].ArrayRange())
        {
            TArray<FAnimTransitionCondition> Conditions;
            if (TransitionNode.hasKey("conditions"))
            {
                for (json::JSON& ConditionNode : TransitionNode["conditions"].ArrayRange())
                {
                    Conditions.push_back(ParseJsonCondition(ConditionNode));
                }
            }
            else
            {
                Conditions.push_back(ParseJsonCondition(TransitionNode));
            }

            Asset->AddTransition(
                FName(GetJsonString(TransitionNode, "from")),
                FName(GetJsonString(TransitionNode, "to")),
                Conditions,
                GetJsonFloat(TransitionNode, "blendTime", 0.0f),
                GetJsonInt(TransitionNode, "priority", 0),
                GetJsonEaseOption(TransitionNode, "easeOption"));
        }
    }

    FString ValidationMessage;
    if (!Asset->Validate(&ValidationMessage))
    {
        UE_LOG_ERROR("[AnimSM] Failed to validate state machine asset %s: %s", NormalizedPath.c_str(), ValidationMessage.c_str());
        UObjectManager::Get().DestroyObject(Asset);
        return nullptr;
    }

    UE_LOG("[AnimSM] Loaded state machine asset: %s", NormalizedPath.c_str());
    return Asset;
}

bool UAnimStateMachineAsset::HasDuplicateTransition(
    const FName& FromState,
    const FName& ToState,
    const TArray<FAnimTransitionCondition>& Conditions) const
{
    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        if (Transition.FromState == FromState &&
            Transition.ToState == ToState &&
            AreConditionsEqual(Transition.Conditions, Conditions))
        {
            return true;
        }
    }

    return false;
}
