#pragma once

#include <cmath>
#include <algorithm>

#include "UGameObject.h"

#include "FVector.h"
#include "FColor.h"

inline const float Pi = 3.1415926535f;

// ?꾪삎 湲곕낯 ?대옒??
class UDiagram : public UGameObject
{
public:
    virtual ~UDiagram() {}


    // 踰?異⑸룎 ?곸슜
    virtual void ApplyWallCollision() = 0;

    // 以묐젰 ?곸슜
    virtual void ApplyGravity(float deltaTime, const FVector& gravity) = 0;

    // ?ㅻⅨ ?꾪삎怨?異⑸룎 ?먯젙
    virtual bool CheckCollision(const UDiagram* Other) = 0;
};
