#pragma once

#include "Core/Containers/Array.h"
#include "Math/Matrix.h"
#include "Object/Object.h"

// Common interface for animation assets that can be played by an anim instance.
class UAnimationAssetBase : public UObject
{
public:
    DECLARE_CLASS(UAnimationAssetBase, UObject)

    virtual float GetPlayLength() const { return 0.0f; }
    virtual bool SamplePose(float Time, TArray<FMatrix>& OutLocalPose) const { return false; }
};
