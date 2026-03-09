#pragma once

#include <cmath>

#include "UGameObject.h"
#include "URenderer.h"
#include "FVector.h"

// 도형 기본 클래스
class UDiagram : public UGameObject
{
public:
    virtual ~UDiagram() {}

    // 물리/이동 업데이트
    virtual void Update(float deltaTime) = 0;

    // 렌더링
    virtual void Render(URenderer& renderer) = 0;

    // 벽 충돌 적용
    virtual void ApplyWallCollision() = 0;

    // 중력 적용
    virtual void ApplyGravity(float deltaTime, const FVector& gravity) = 0;

    virtual bool CheckCollision(const UDiagram* Other) = 0;
};