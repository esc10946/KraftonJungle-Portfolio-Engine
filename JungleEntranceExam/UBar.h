#pragma once

#include "UDiagram.h"

enum EDirection
{
    Left = -1,
    Right = 1
};

class UBar : public UDiagram
{
public:
    FVector Location;           // 바의 위치
    float Speed;                // 바의 이동속도
    float Scale;
    float XLength;               // 바의 가로 절반길이
    float YLength;               // 바의 세로 절반길이
    int PlayerNo;
    int Direction;

    // 생성자 및 소멸자
public:
    UBar(const FVector& _Location, const float _Speed, const float _Scale, int _PlayerNo);

    virtual ~UBar() override;

    // UPrimitive 인터페이스 구현
    // 물리/이동 업데이트
    virtual void Update(float deltaTime);

    // 렌더링 (상수 버퍼 업데이트)
    virtual void Render(URenderer& renderer);

    // 벽 충돌 적용
    virtual void ApplyWallCollision();

    // 충격량 적용
    virtual void ApplyGravity(float deltaTime, const FVector& gravity);

    virtual bool CheckCollision(const UDiagram* Other) override;

    // void ResolveCollision(UBar* Other);

    void SetScale(const float _Scale);
};