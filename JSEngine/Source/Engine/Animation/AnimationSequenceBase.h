#pragma once

#include "Core/Containers/Array.h"
#include "Math/Matrix.h"
#include "Object/Object.h"

class USkeletalMesh;

// Common interface for animation sequences that can be played by an anim instance.
class UAnimationSequenceBase : public UObject
{
public:
    DECLARE_CLASS(UAnimationSequenceBase, UObject)

    virtual float GetPlayLength() const { return 0.0f; }
    virtual bool SamplePose(const USkeletalMesh* TargetMesh, float Time, TArray<FMatrix>& OutLocalPose) const { return false; }
};
