#pragma once

#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/CoreTypes.h"
#include "Math/Vector.h"
#include "Object/FName.h"

using FAnimStateId = uint32;
using FAnimTransitionId = uint32;

static constexpr FAnimStateId InvalidAnimStateId = 0;
static constexpr FAnimTransitionId InvalidAnimTransitionId = 0;

enum class EAnimBlendEaseOption : uint8
{
    Linear = 0,
    EaseIn,
    EaseOut,
    EaseInOut,
};

enum class EAnimParameterType : uint8
{
    Bool = 0,
    Int,
    Float,
    Vector,
    Trigger,
};

enum class EAnimCompareOp : uint8
{
    Equal = 0,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    IsTrue,
    IsFalse,
};

struct FAnimParameterValue
{
    bool BoolValue = false;
    int32 IntValue = 0;
    float FloatValue = 0.0f;
    FVector VectorValue;
};

struct FAnimStateMachineParameterDesc
{
    FName Name;
    EAnimParameterType Type = EAnimParameterType::Bool;
};

struct FAnimTransitionCondition
{
    FName ParameterName;
    EAnimCompareOp CompareOp = EAnimCompareOp::Equal;
    FAnimParameterValue CompareValue;
};

struct FAnimStateDesc
{
    FAnimStateId Id = InvalidAnimStateId;
    FName StateName;
    FName AnimationName;
    FString AnimationPath;
    bool bLoop = true;
};

struct FAnimTransitionDesc
{
    FAnimTransitionId Id = InvalidAnimTransitionId;
    FAnimStateId FromStateId = InvalidAnimStateId;
    FAnimStateId ToStateId = InvalidAnimStateId;
    FName FromState;
    FName ToState;
    TArray<FAnimTransitionCondition> Conditions;
    float BlendTime = 0.0f;
    EAnimBlendEaseOption EaseOption = EAnimBlendEaseOption::Linear;
    int32 Priority = 0;
};

struct FAnimStateEditorMetadata
{
    FAnimStateId StateId = InvalidAnimStateId;
    float NodeX = 0.0f;
    float NodeY = 0.0f;
    float NodeWidth = 160.0f;
    float NodeHeight = 80.0f;
    FString Comment;
    uint32 Color = 0;
};

struct FAnimTransitionEditorMetadata
{
    FAnimTransitionId TransitionId = InvalidAnimTransitionId;
    float ControlPointX = 0.0f;
    float ControlPointY = 0.0f;
    FString Comment;
};
