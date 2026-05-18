#pragma once

#include "Core/Containers/Array.h"
#include "Math/Matrix.h"
#include "Object/FName.h"
#include "Object/Object.h"

class USkeletalMesh;

struct FAnimNotifyEvent
{
    float TriggerTime = 0.0f;
    float Duration = 0.0f;
    FName NotifyName;
};

#include "UAnimationSequenceBase.generated.h"
// Common interface for animation sequences that can be played by an anim instance.
UCLASS()
class UAnimationSequenceBase : public UObject
{
public:
    GENERATED_BODY(UAnimationSequenceBase, UObject)

    virtual float GetPlayLength() const { return 0.0f; }
    virtual bool SamplePose(const USkeletalMesh* TargetMesh, float Time, TArray<FMatrix>& OutLocalPose) const { return false; }

    void AddNotify(const FAnimNotifyEvent& Notify);
    bool RemoveNotifyAt(int32 NotifyIndex);
    const TArray<FAnimNotifyEvent>& GetNotifies() const { return Notifies; }

protected:
    TArray<FAnimNotifyEvent> Notifies;
};
