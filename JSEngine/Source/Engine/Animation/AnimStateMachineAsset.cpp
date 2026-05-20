#include "Animation/AnimStateMachineAsset.h"

#include "Animation/AnimStateMachineSerializer.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Object/Object.h"

#include <algorithm>

namespace
{
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

bool IsCompareOpValidForParameterType(EAnimParameterType ParameterType, EAnimCompareOp CompareOp)
{
    switch (ParameterType)
    {
    case EAnimParameterType::Bool:
        return CompareOp == EAnimCompareOp::Equal ||
            CompareOp == EAnimCompareOp::NotEqual ||
            CompareOp == EAnimCompareOp::IsTrue ||
            CompareOp == EAnimCompareOp::IsFalse;
    case EAnimParameterType::Int:
    case EAnimParameterType::Float:
        return CompareOp == EAnimCompareOp::Equal ||
            CompareOp == EAnimCompareOp::NotEqual ||
            CompareOp == EAnimCompareOp::Greater ||
            CompareOp == EAnimCompareOp::GreaterEqual ||
            CompareOp == EAnimCompareOp::Less ||
            CompareOp == EAnimCompareOp::LessEqual;
    case EAnimParameterType::Vector:
        return CompareOp == EAnimCompareOp::Equal ||
            CompareOp == EAnimCompareOp::NotEqual;
    case EAnimParameterType::Trigger:
        return CompareOp == EAnimCompareOp::IsTrue;
    default:
        return false;
    }
}

EAnimCompareOp GetDefaultCompareOpForParameterType(EAnimParameterType ParameterType)
{
    return ParameterType == EAnimParameterType::Trigger ? EAnimCompareOp::IsTrue : EAnimCompareOp::Equal;
}

void ResetConditionForParameterType(FAnimTransitionCondition& Condition, EAnimParameterType ParameterType)
{
    Condition.CompareOp = GetDefaultCompareOpForParameterType(ParameterType);
    Condition.CompareValue = FAnimParameterValue();
}
} // namespace

void UAnimStateMachineAsset::Clear()
{
    EntryState = FName();
    EntryStateId = InvalidAnimStateId;
    Parameters.clear();
    States.clear();
    Transitions.clear();
    StateEditorMetadata.clear();
    TransitionEditorMetadata.clear();
}

void UAnimStateMachineAsset::SetEntryState(const FName& StateName)
{
    EntryState = StateName;
    const FAnimStateDesc* State = FindState(StateName);
    EntryStateId = State ? State->Id : InvalidAnimStateId;
}

bool UAnimStateMachineAsset::SetEntryStateId(FAnimStateId StateId)
{
    const FAnimStateDesc* State = FindStateById(StateId);
    if (!State)
    {
        EntryState = FName();
        EntryStateId = InvalidAnimStateId;
        return false;
    }

    EntryStateId = StateId;
    EntryState = State->StateName;
    return true;
}

bool UAnimStateMachineAsset::AddState(
    const FName& StateName,
    const FName& AnimationName,
    bool bLoop,
    const FString& AnimationPath)
{
    return AddStateWithId(GenerateStateId(), StateName, AnimationName, bLoop, AnimationPath);
}

bool UAnimStateMachineAsset::AddStateWithId(
    FAnimStateId StateId,
    const FName& StateName,
    const FName& AnimationName,
    bool bLoop,
    const FString& AnimationPath)
{
    if (StateId == InvalidAnimStateId || !StateName.IsValid())
    {
        UE_LOG_WARNING("[AnimSM] Invalid state: empty id or state name");
        return false;
    }

    if (FindStateById(StateId))
    {
        UE_LOG_WARNING("[AnimSM] Invalid state: duplicate state id %u", StateId);
        return false;
    }

    if (FindState(StateName))
    {
        UE_LOG_WARNING("[AnimSM] Invalid state: duplicate state %s", StateName.ToString().c_str());
        return false;
    }

    FAnimStateDesc State;
    State.Id = StateId;
    State.StateName = StateName;
    State.AnimationName = AnimationName;
    State.AnimationPath = FPaths::Normalize(AnimationPath);
    State.bLoop = bLoop;
    States.push_back(State);

    if (EntryState == StateName || EntryStateId == StateId)
    {
        EntryState = StateName;
        EntryStateId = StateId;
    }

    return true;
}

bool UAnimStateMachineAsset::SetStateAnimationPath(const FName& StateName, const FString& AnimationPath)
{
    if (!StateName.IsValid() || AnimationPath.empty())
    {
        return false;
    }

    if (FAnimStateDesc* State = FindState(StateName))
    {
        State->AnimationPath = FPaths::Normalize(AnimationPath);
        return true;
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
    const FAnimStateDesc* FromStateDesc = FindState(FromState);
    const FAnimStateDesc* ToStateDesc = FindState(ToState);
    if (!FromStateDesc || !ToStateDesc)
    {
        UE_LOG_WARNING(
            "[AnimSM] Invalid transition: missing from/to state %s -> %s",
            FromState.ToString().c_str(),
            ToState.ToString().c_str());
        return false;
    }

    return AddTransitionWithId(
        GenerateTransitionId(),
        FromStateDesc->Id,
        ToStateDesc->Id,
        Conditions,
        BlendTime,
        Priority,
        EaseOption);
}

bool UAnimStateMachineAsset::AddTransitionWithId(
    FAnimTransitionId TransitionId,
    FAnimStateId FromStateId,
    FAnimStateId ToStateId,
    const TArray<FAnimTransitionCondition>& Conditions,
    float BlendTime,
    int32 Priority,
    EAnimBlendEaseOption EaseOption)
{
    const FAnimStateDesc* FromStateDesc = FindStateById(FromStateId);
    const FAnimStateDesc* ToStateDesc = FindStateById(ToStateId);
    if (TransitionId == InvalidAnimTransitionId || !FromStateDesc || !ToStateDesc || Conditions.empty())
    {
        UE_LOG_WARNING("[AnimSM] Invalid transition: empty id/from/to/conditions");
        return false;
    }

    if (FindTransitionById(TransitionId))
    {
        UE_LOG_WARNING("[AnimSM] Invalid transition: duplicate transition id %u", TransitionId);
        return false;
    }

    for (const FAnimTransitionCondition& Condition : Conditions)
    {
        FString ValidationMessage;
        if (!IsConditionCompatibleWithDeclaration(Condition, &ValidationMessage))
        {
            UE_LOG_WARNING("[AnimSM] Invalid transition condition: %s", ValidationMessage.c_str());
            return false;
        }
    }

    if (HasDuplicateTransition(FromStateId, ToStateId, Conditions))
    {
        UE_LOG_WARNING(
            "[AnimSM] Invalid transition: duplicate %s -> %s",
            FromStateDesc->StateName.ToString().c_str(),
            ToStateDesc->StateName.ToString().c_str());
        return false;
    }

    FAnimTransitionDesc Transition;
    Transition.Id = TransitionId;
    Transition.FromStateId = FromStateId;
    Transition.ToStateId = ToStateId;
    Transition.FromState = FromStateDesc->StateName;
    Transition.ToState = ToStateDesc->StateName;
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
    if (!EnsureParameterDeclaration(ParameterName, EAnimParameterType::Bool))
    {
        return false;
    }

    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
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
    if (!EnsureParameterDeclaration(ParameterName, EAnimParameterType::Int))
    {
        return false;
    }

    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
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
    if (!EnsureParameterDeclaration(ParameterName, EAnimParameterType::Float))
    {
        return false;
    }

    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
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
    if (!EnsureParameterDeclaration(ParameterName, EAnimParameterType::Vector))
    {
        return false;
    }

    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
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
    if (!EnsureParameterDeclaration(ParameterName, EAnimParameterType::Trigger))
    {
        return false;
    }

    FAnimTransitionCondition Condition;
    Condition.ParameterName = ParameterName;
    Condition.CompareOp = EAnimCompareOp::IsTrue;

    TArray<FAnimTransitionCondition> Conditions;
    Conditions.push_back(Condition);
    return AddTransition(FromState, ToState, Conditions, BlendTime, Priority, EaseOption);
}

bool UAnimStateMachineAsset::AddParameter(const FName& ParameterName, EAnimParameterType ParameterType)
{
    if (!ParameterName.IsValid())
    {
        UE_LOG_WARNING("[AnimSM] Invalid parameter: empty name");
        return false;
    }

    if (FAnimStateMachineParameterDesc* ExistingParameter = FindParameter(ParameterName))
    {
        if (ExistingParameter->Type == ParameterType)
        {
            return true;
        }

        UE_LOG_WARNING(
            "[AnimSM] Invalid parameter: type conflict for %s",
            ParameterName.ToString().c_str());
        return false;
    }

    FAnimStateMachineParameterDesc Parameter;
    Parameter.Name = ParameterName;
    Parameter.Type = ParameterType;
    Parameters.push_back(Parameter);
    return true;
}

bool UAnimStateMachineAsset::RenameParameter(const FName& OldParameterName, const FName& NewParameterName)
{
    if (!OldParameterName.IsValid() || !NewParameterName.IsValid() || OldParameterName == NewParameterName)
    {
        return false;
    }

    FAnimStateMachineParameterDesc* Parameter = FindParameter(OldParameterName);
    if (!Parameter || FindParameter(NewParameterName))
    {
        return false;
    }

    Parameter->Name = NewParameterName;
    for (FAnimTransitionDesc& Transition : Transitions)
    {
        for (FAnimTransitionCondition& Condition : Transition.Conditions)
        {
            if (Condition.ParameterName == OldParameterName)
            {
                Condition.ParameterName = NewParameterName;
            }
        }
    }

    return true;
}

bool UAnimStateMachineAsset::SetParameterType(const FName& ParameterName, EAnimParameterType ParameterType)
{
    FAnimStateMachineParameterDesc* Parameter = FindParameter(ParameterName);
    if (!Parameter)
    {
        return false;
    }

    if (Parameter->Type == ParameterType)
    {
        return true;
    }

    Parameter->Type = ParameterType;
    for (FAnimTransitionDesc& Transition : Transitions)
    {
        for (FAnimTransitionCondition& Condition : Transition.Conditions)
        {
            if (Condition.ParameterName == ParameterName)
            {
                ResetConditionForParameterType(Condition, ParameterType);
            }
        }
    }

    return true;
}

bool UAnimStateMachineAsset::RemoveParameter(const FName& ParameterName)
{
    if (IsParameterReferenced(ParameterName))
    {
        UE_LOG_WARNING(
            "[AnimSM] Remove parameter failed: parameter is referenced %s",
            ParameterName.ToString().c_str());
        return false;
    }

    const auto OldSize = Parameters.size();
    Parameters.erase(
        std::remove_if(
            Parameters.begin(),
            Parameters.end(),
            [&ParameterName](const FAnimStateMachineParameterDesc& Parameter)
            {
                return Parameter.Name == ParameterName;
            }),
        Parameters.end());
    return Parameters.size() != OldSize;
}

bool UAnimStateMachineAsset::RemoveParameterAndConditions(const FName& ParameterName)
{
    if (!FindParameter(ParameterName))
    {
        return false;
    }

    Parameters.erase(
        std::remove_if(
            Parameters.begin(),
            Parameters.end(),
            [&ParameterName](const FAnimStateMachineParameterDesc& ExistingParameter)
            {
                return ExistingParameter.Name == ParameterName;
            }),
        Parameters.end());

    for (FAnimTransitionDesc& Transition : Transitions)
    {
        Transition.Conditions.erase(
            std::remove_if(
                Transition.Conditions.begin(),
                Transition.Conditions.end(),
                [&ParameterName](const FAnimTransitionCondition& Condition)
                {
                    return Condition.ParameterName == ParameterName;
                }),
            Transition.Conditions.end());
    }

    Transitions.erase(
        std::remove_if(
            Transitions.begin(),
            Transitions.end(),
            [](const FAnimTransitionDesc& Transition)
            {
                return Transition.Conditions.empty();
            }),
        Transitions.end());
    return true;
}

uint32 UAnimStateMachineAsset::RemoveUnusedParameters()
{
    const auto OldSize = Parameters.size();
    Parameters.erase(
        std::remove_if(
            Parameters.begin(),
            Parameters.end(),
            [this](const FAnimStateMachineParameterDesc& Parameter)
            {
                return !IsParameterReferenced(Parameter.Name);
            }),
        Parameters.end());
    return static_cast<uint32>(OldSize - Parameters.size());
}

const FAnimStateMachineParameterDesc* UAnimStateMachineAsset::FindParameter(const FName& ParameterName) const
{
    if (!ParameterName.IsValid())
    {
        return nullptr;
    }

    for (const FAnimStateMachineParameterDesc& Parameter : Parameters)
    {
        if (Parameter.Name == ParameterName)
        {
            return &Parameter;
        }
    }

    return nullptr;
}

FAnimStateMachineParameterDesc* UAnimStateMachineAsset::FindParameter(const FName& ParameterName)
{
    return const_cast<FAnimStateMachineParameterDesc*>(
        static_cast<const UAnimStateMachineAsset*>(this)->FindParameter(ParameterName));
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

FAnimStateDesc* UAnimStateMachineAsset::FindState(const FName& StateName)
{
    return const_cast<FAnimStateDesc*>(static_cast<const UAnimStateMachineAsset*>(this)->FindState(StateName));
}

const FAnimStateDesc* UAnimStateMachineAsset::FindStateById(FAnimStateId StateId) const
{
    if (StateId == InvalidAnimStateId)
    {
        return nullptr;
    }

    for (const FAnimStateDesc& State : States)
    {
        if (State.Id == StateId)
        {
            return &State;
        }
    }

    return nullptr;
}

FAnimStateDesc* UAnimStateMachineAsset::FindStateById(FAnimStateId StateId)
{
    return const_cast<FAnimStateDesc*>(static_cast<const UAnimStateMachineAsset*>(this)->FindStateById(StateId));
}

const FAnimTransitionDesc* UAnimStateMachineAsset::FindTransitionById(FAnimTransitionId TransitionId) const
{
    if (TransitionId == InvalidAnimTransitionId)
    {
        return nullptr;
    }

    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        if (Transition.Id == TransitionId)
        {
            return &Transition;
        }
    }

    return nullptr;
}

TArray<const FAnimTransitionDesc*> UAnimStateMachineAsset::GetTransitionsFrom(const FName& StateName) const
{
    const FAnimStateDesc* State = FindState(StateName);
    return State ? GetTransitionsFrom(State->Id) : TArray<const FAnimTransitionDesc*>();
}

TArray<const FAnimTransitionDesc*> UAnimStateMachineAsset::GetTransitionsFrom(FAnimStateId StateId) const
{
    TArray<const FAnimTransitionDesc*> Result;
    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        if (Transition.FromStateId == StateId)
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

    if (EntryStateId == InvalidAnimStateId || !FindStateById(EntryStateId))
    {
        SetMessage("Entry state is missing or does not exist");
        UE_LOG_WARNING("[AnimSM] Invalid asset: entry state is missing or does not exist");
        return false;
    }

    for (size_t StateIndex = 0; StateIndex < States.size(); ++StateIndex)
    {
        const FAnimStateDesc& State = States[StateIndex];
        if (State.Id == InvalidAnimStateId || !State.StateName.IsValid())
        {
            SetMessage("State id or name is empty");
            UE_LOG_WARNING("[AnimSM] Invalid asset: state id or name is empty");
            return false;
        }

        for (size_t OtherIndex = StateIndex + 1; OtherIndex < States.size(); ++OtherIndex)
        {
            if (State.Id == States[OtherIndex].Id)
            {
                SetMessage("Duplicate state id");
                UE_LOG_WARNING("[AnimSM] Invalid asset: duplicate state id %u", State.Id);
                return false;
            }
            if (State.StateName == States[OtherIndex].StateName)
            {
                SetMessage("Duplicate state name");
                UE_LOG_WARNING("[AnimSM] Invalid asset: duplicate state %s", State.StateName.ToString().c_str());
                return false;
            }
        }
    }

    for (size_t ParameterIndex = 0; ParameterIndex < Parameters.size(); ++ParameterIndex)
    {
        const FAnimStateMachineParameterDesc& Parameter = Parameters[ParameterIndex];
        if (!Parameter.Name.IsValid())
        {
            SetMessage("Parameter name is empty");
            UE_LOG_WARNING("[AnimSM] Invalid asset: parameter name is empty");
            return false;
        }

        for (size_t OtherIndex = ParameterIndex + 1; OtherIndex < Parameters.size(); ++OtherIndex)
        {
            if (Parameter.Name == Parameters[OtherIndex].Name)
            {
                SetMessage("Duplicate parameter name");
                UE_LOG_WARNING("[AnimSM] Invalid asset: duplicate parameter %s", Parameter.Name.ToString().c_str());
                return false;
            }
        }
    }

    for (size_t TransitionIndex = 0; TransitionIndex < Transitions.size(); ++TransitionIndex)
    {
        const FAnimTransitionDesc& Transition = Transitions[TransitionIndex];
        if (Transition.Id == InvalidAnimTransitionId)
        {
            SetMessage("Transition id is empty");
            UE_LOG_WARNING("[AnimSM] Invalid asset: transition id is empty");
            return false;
        }

        for (size_t OtherIndex = TransitionIndex + 1; OtherIndex < Transitions.size(); ++OtherIndex)
        {
            if (Transition.Id == Transitions[OtherIndex].Id)
            {
                SetMessage("Duplicate transition id");
                UE_LOG_WARNING("[AnimSM] Invalid asset: duplicate transition id %u", Transition.Id);
                return false;
            }
        }

        if (!FindStateById(Transition.FromStateId))
        {
            SetMessage("Transition from-state does not exist");
            UE_LOG_WARNING("[AnimSM] Invalid asset: from-state id does not exist %u", Transition.FromStateId);
            return false;
        }

        if (!FindStateById(Transition.ToStateId))
        {
            SetMessage("Transition to-state does not exist");
            UE_LOG_WARNING("[AnimSM] Invalid asset: to-state id does not exist %u", Transition.ToStateId);
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
            FString ConditionMessage;
            if (!IsConditionCompatibleWithDeclaration(Condition, &ConditionMessage))
            {
                SetMessage(ConditionMessage);
                UE_LOG_WARNING("[AnimSM] Invalid asset: %s", ConditionMessage.c_str());
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

bool UAnimStateMachineAsset::SaveToFile(const FString& Path) const
{
    FAnimStateMachineSerializer Serializer;
    return Serializer.Save(Path, *this);
}

UAnimStateMachineAsset* UAnimStateMachineAsset::LoadFromFile(const FString& Path)
{
    UAnimStateMachineAsset* Asset = UObjectManager::Get().CreateObject<UAnimStateMachineAsset>();
    FAnimStateMachineSerializer Serializer;
    if (!Serializer.Load(Path, *Asset))
    {
        UObjectManager::Get().DestroyObject(Asset);
        return nullptr;
    }

    return Asset;
}

bool UAnimStateMachineAsset::ReadAnimationDependenciesFromFile(const FString& Path, TArray<FString>& OutAnimationPaths)
{
    FAnimStateMachineSerializer Serializer;
    return Serializer.ReadAnimationDependencies(Path, OutAnimationPaths);
}

FAnimStateId UAnimStateMachineAsset::GenerateStateId() const
{
    FAnimStateId MaxId = InvalidAnimStateId;
    for (const FAnimStateDesc& State : States)
    {
        MaxId = std::max(MaxId, State.Id);
    }
    return MaxId + 1;
}

FAnimTransitionId UAnimStateMachineAsset::GenerateTransitionId() const
{
    FAnimTransitionId MaxId = InvalidAnimTransitionId;
    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        MaxId = std::max(MaxId, Transition.Id);
    }
    return MaxId + 1;
}

bool UAnimStateMachineAsset::HasDuplicateTransition(
    FAnimStateId FromStateId,
    FAnimStateId ToStateId,
    const TArray<FAnimTransitionCondition>& Conditions) const
{
    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        if (Transition.FromStateId == FromStateId &&
            Transition.ToStateId == ToStateId &&
            AreConditionsEqual(Transition.Conditions, Conditions))
        {
            return true;
        }
    }

    return false;
}

bool UAnimStateMachineAsset::IsParameterReferenced(const FName& ParameterName) const
{
    if (!ParameterName.IsValid())
    {
        return false;
    }

    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        for (const FAnimTransitionCondition& Condition : Transition.Conditions)
        {
            if (Condition.ParameterName == ParameterName)
            {
                return true;
            }
        }
    }

    return false;
}

bool UAnimStateMachineAsset::IsConditionCompatibleWithDeclaration(
    const FAnimTransitionCondition& Condition,
    FString* OutMessage) const
{
    auto SetMessage = [OutMessage](const FString& Message)
    {
        if (OutMessage)
        {
            *OutMessage = Message;
        }
    };

    if (!Condition.ParameterName.IsValid())
    {
        SetMessage("Transition condition parameter name is empty");
        return false;
    }

    const FAnimStateMachineParameterDesc* Parameter = FindParameter(Condition.ParameterName);
    if (!Parameter)
    {
        SetMessage("Transition condition parameter is not declared");
        return false;
    }

    if (!IsCompareOpValidForParameterType(Parameter->Type, Condition.CompareOp))
    {
        SetMessage("Transition condition compare op does not match parameter type");
        return false;
    }

    return true;
}

bool UAnimStateMachineAsset::EnsureParameterDeclaration(const FName& ParameterName, EAnimParameterType ParameterType)
{
    return AddParameter(ParameterName, ParameterType);
}
