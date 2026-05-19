#pragma once

#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/CoreTypes.h"
#include "Math/Vector.h"
#include "Object/FName.h"

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

struct FAnimTransitionCondition
{
    FName ParameterName;
    EAnimParameterType ParameterType = EAnimParameterType::Bool;
    EAnimCompareOp CompareOp = EAnimCompareOp::Equal;
    FAnimParameterValue CompareValue;
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
    TArray<FAnimTransitionCondition> Conditions;
    float BlendTime = 0.0f;
    EAnimBlendEaseOption EaseOption = EAnimBlendEaseOption::Linear;
    int32 Priority = 0;
};
