#pragma once

#include "Asset/CurveFloatAsset.h"
#include "Core/CoreMinimal.h"

enum class EAnimationTrackType : uint8
{
    BoneTransform = 0,
    ShapeKey,
    MaterialParameter,
    Camera,
    CustomProperty
};

enum class EAnimationCurveValueType : uint8
{
    Float = 0,
    Vector2,
    Vector3,
    Quaternion,
    Transform
};

enum class EAnimationRotationOrder : uint8
{
    XYZ = 0,
    XZY,
    YZX,
    YXZ,
    ZXY,
    ZYX,
    Spheric
};

enum class EAnimationCurveChannel : uint8
{
    TranslationX = 0,
    TranslationY,
    TranslationZ,
    RotationX,
    RotationY,
    RotationZ,
    ScaleX,
    ScaleY,
    ScaleZ,
    ShapeWeight,
    CustomFloat
};

struct FAnimationCurveKey
{
    float TimeSeconds = 0.0f;
    float Value = 0.0f;

    ECurveInterpMode InterpMode = ECurveInterpMode::Linear;
    ECurveTangentMode TangentMode = ECurveTangentMode::Auto;

    float ArriveTangent = 0.0f;
    float LeaveTangent = 0.0f;

    float ArriveTangentWeight = 0.0f;
    float LeaveTangentWeight = 0.0f;
    float ArriveTangentVelocity = 0.0f;
    float LeaveTangentVelocity = 0.0f;

    bool bArriveWeighted = false;
    bool bLeaveWeighted = false;
    bool bArriveHasVelocity = false;
    bool bLeaveHasVelocity = false;

    int32 RawInterpolation = 0;
    int32 RawConstantMode = 0;
    int32 RawTangentMode = 0;
};

struct FAnimationFloatCurve
{
    EAnimationCurveChannel Channel = EAnimationCurveChannel::CustomFloat;
    TArray<FAnimationCurveKey> Keys;
    float DefaultValue = 0.0f;
    bool bHasCurve = false;
};

struct FBoneAnimationTrack
{
    FString BoneName;

    FVector DefaultTranslation = FVector::ZeroVector;
    FVector DefaultRotationEuler = FVector::ZeroVector;
    FVector DefaultScale = FVector(1.0f, 1.0f, 1.0f);

    EAnimationRotationOrder RotationOrder = EAnimationRotationOrder::XYZ;
    int32 TransformInheritType = 0;
    bool bRotationActive = false;

    FVector PreRotation = FVector::ZeroVector;
    FVector PostRotation = FVector::ZeroVector;
    FVector RotationOffset = FVector::ZeroVector;
    FVector RotationPivot = FVector::ZeroVector;
    FVector ScalingOffset = FVector::ZeroVector;
    FVector ScalingPivot = FVector::ZeroVector;

    FAnimationFloatCurve Translation[3];
    FAnimationFloatCurve Rotation[3];
    FAnimationFloatCurve Scale[3];
};

// 후속 확장용: 1차 구현에서는 import/evaluation 대상에서 제외한다.
struct FShapeKeyAnimationTrack
{
    FString MeshNodeName;
    FString ShapeKeyName;
    int32 ShapeKeyIndex = -1;

    FAnimationFloatCurve WeightCurve;
};

struct FAnimationClip
{
    FString Name;
    FString SourcePath;
    FString SkeletonSourcePath;

    float StartTimeSeconds = 0.0f;
    float EndTimeSeconds = 0.0f;
    float DurationSeconds = 0.0f;
    float SourceFrameRate = 0.0f;

    TArray<FBoneAnimationTrack> BoneTracks;

    // 후속 확장용: shape key import/evaluation 구현 시 활성화한다.
    TArray<FShapeKeyAnimationTrack> ShapeKeyTracks;
};

