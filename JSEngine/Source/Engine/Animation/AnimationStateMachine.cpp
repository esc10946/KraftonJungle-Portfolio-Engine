#include "Animation/AnimationStateMachine.h"

#include "Animation/AnimInstanceBase.h"
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

bool IsConditionNameKnown(const FName& ConditionName)
{
    return ConditionName == FName("CanWalk") ||
           ConditionName == FName("ShouldIdle") ||
           ConditionName == FName("CanRun") ||
           ConditionName == FName("IsFalling") ||
           ConditionName == FName("IsFlying");
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
    const FName& ConditionName,
    float BlendTime,
    int32 Priority,
    EAnimBlendEaseOption EaseOption)
{
    if (!FromState.IsValid() || !ToState.IsValid() || !ConditionName.IsValid())
    {
        UE_LOG_WARNING("[AnimSM] Invalid transition: empty from/to/condition");
        return false;
    }

    if (HasDuplicateTransition(FromState, ToState, ConditionName))
    {
        UE_LOG_WARNING(
            "[AnimSM] Invalid transition: duplicate %s -> %s (%s)",
            FromState.ToString().c_str(),
            ToState.ToString().c_str(),
            ConditionName.ToString().c_str());
        return false;
    }

    FAnimTransitionDesc Transition;
    Transition.FromState = FromState;
    Transition.ToState = ToState;
    Transition.ConditionName = ConditionName;
    Transition.BlendTime = std::max(0.0f, BlendTime);
    Transition.EaseOption = EaseOption;
    Transition.Priority = Priority;
    Transitions.push_back(Transition);
    return true;
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

        if (!Transition.ConditionName.IsValid())
        {
            SetMessage("Transition condition name is empty");
            UE_LOG_WARNING("[AnimSM] Invalid asset: transition condition name is empty");
            return false;
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
            Asset->AddTransition(
                FName(GetJsonString(TransitionNode, "from")),
                FName(GetJsonString(TransitionNode, "to")),
                FName(GetJsonString(TransitionNode, "condition")),
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
    const FName& ConditionName) const
{
    for (const FAnimTransitionDesc& Transition : Transitions)
    {
        if (Transition.FromState == FromState &&
            Transition.ToState == ToState &&
            Transition.ConditionName == ConditionName)
        {
            return true;
        }
    }

    return false;
}

void FAnimStateMachineNode::Initialize(UAnimStateMachineAsset* InAsset, UAnimInstanceBase* InAnimInstance)
{
    Asset = InAsset;
    AnimInstance = InAnimInstance;
    CurrentState = FName();
    PreviousState = FName();
    StateElapsedTime = 0.0f;
    WarnedMissingConditions.clear();

    if (!Asset)
    {
        return;
    }

    FString ValidationMessage;
    if (!Asset->Validate(&ValidationMessage))
    {
        UE_LOG_WARNING("[AnimSM] Runtime initialize failed: %s", ValidationMessage.c_str());
        return;
    }

    ChangeState(Asset->GetEntryState(), 0.0f, EAnimBlendEaseOption::Linear);
}

void FAnimStateMachineNode::Update(float DeltaSeconds, const FAnimStateMachineContext& Context)
{
    if (!Asset || !AnimInstance || !CurrentState.IsValid())
    {
        return;
    }

    StateElapsedTime += DeltaSeconds;

    TArray<const FAnimTransitionDesc*> TransitionsFromCurrent = Asset->GetTransitionsFrom(CurrentState);
    for (const FAnimTransitionDesc* Transition : TransitionsFromCurrent)
    {
        if (!Transition)
        {
            continue;
        }

        if (EvaluateCondition(Transition->ConditionName, Context))
        {
            ChangeState(Transition->ToState, Transition->BlendTime, Transition->EaseOption);
            return;
        }
    }
}

void FAnimStateMachineNode::Reset()
{
    UAnimStateMachineAsset* ExistingAsset = Asset;
    UAnimInstanceBase* ExistingAnimInstance = AnimInstance;
    Asset = nullptr;
    AnimInstance = nullptr;
    CurrentState = FName();
    PreviousState = FName();
    StateElapsedTime = 0.0f;
    WarnedMissingConditions.clear();

    if (ExistingAsset && ExistingAnimInstance)
    {
        Initialize(ExistingAsset, ExistingAnimInstance);
    }
}

void FAnimStateMachineNode::ChangeState(
    const FName& NewState,
    float BlendTime,
    EAnimBlendEaseOption EaseOption)
{
    if (!Asset || !AnimInstance || !NewState.IsValid() || CurrentState == NewState)
    {
        return;
    }

    const FAnimStateDesc* State = Asset->FindState(NewState);
    if (!State)
    {
        UE_LOG_WARNING("[AnimSM] Missing state: %s", NewState.ToString().c_str());
        return;
    }

    if (!State->AnimationName.IsValid())
    {
        UE_LOG_WARNING("[AnimSM] Missing animation for state: %s", NewState.ToString().c_str());
        return;
    }

    const FName OldState = CurrentState;
    if (!AnimInstance->BlendToAnimationByName(State->AnimationName, State->bLoop, BlendTime, EaseOption))
    {
        UE_LOG_WARNING(
            "[AnimSM] Missing animation mapping: state=%s animation=%s",
            State->StateName.ToString().c_str(),
            State->AnimationName.ToString().c_str());
        return;
    }

    PreviousState = CurrentState;
    CurrentState = NewState;
    StateElapsedTime = 0.0f;

    UE_LOG(
        "[AnimSM] State change: %s -> %s (BlendTime=%.3f)",
        OldState.IsValid() ? OldState.ToString().c_str() : "<None>",
        CurrentState.ToString().c_str(),
        BlendTime);
}

bool FAnimStateMachineNode::EvaluateCondition(
    const FName& ConditionName,
    const FAnimStateMachineContext& Context) const
{
    if (ConditionName == FName("ShouldIdle"))
    {
        return Context.bIsGrounded && Context.Speed < Context.IdleSpeedThreshold;
    }

    if (ConditionName == FName("CanWalk"))
    {
        return Context.bIsGrounded &&
               Context.Speed >= Context.IdleSpeedThreshold &&
               Context.Speed <= Context.WalkSpeed;
    }

    if (ConditionName == FName("CanRun"))
    {
        return Context.bIsGrounded && Context.Speed > Context.WalkSpeed;
    }

    if (ConditionName == FName("IsFalling"))
    {
        return !Context.bIsGrounded ||
               Context.MovementMode == EAnimStateMachineMovementMode::Falling;
    }

    if (ConditionName == FName("IsFlying"))
    {
        return Context.MovementMode == EAnimStateMachineMovementMode::Flying;
    }

    if (!IsConditionNameKnown(ConditionName) && !HasWarnedMissingCondition(ConditionName))
    {
        UE_LOG_WARNING("[AnimSM] Missing condition: %s", ConditionName.ToString().c_str());
        MarkMissingConditionWarned(ConditionName);
    }

    return false;
}

bool FAnimStateMachineNode::HasWarnedMissingCondition(const FName& ConditionName) const
{
    for (const FName& WarnedCondition : WarnedMissingConditions)
    {
        if (WarnedCondition == ConditionName)
        {
            return true;
        }
    }

    return false;
}

void FAnimStateMachineNode::MarkMissingConditionWarned(const FName& ConditionName) const
{
    if (ConditionName.IsValid())
    {
        WarnedMissingConditions.push_back(ConditionName);
    }
}
