#pragma once

#include "Core/Containers/Array.h"
#include "Math/Matrix.h"
#include "Object/Object.h"

class USkeletalMesh;

#include "UAnimationSequenceBase.generated.h"
// Common interface for animation sequences that can be played by an anim instance.
UCLASS()
class UAnimationSequenceBase : public UObject
{
public:
    GENERATED_BODY(UAnimationSequenceBase, UObject)

    virtual float GetPlayLength() const { return 0.0f; }
    virtual bool SamplePose(const USkeletalMesh* TargetMesh, float Time, TArray<FMatrix>& OutLocalPose) const { return false; }
};
