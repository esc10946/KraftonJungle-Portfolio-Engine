#pragma once

#include "Object/Object.h"

class UAnimationAsset : public UObject
{
public:
    DECLARE_CLASS(UAnimationAsset, UObject)

    virtual float GetPlayLength() const { return 0.0f; }
};
