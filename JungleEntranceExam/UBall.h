#pragma once

#include <iostream>
#include <queue>

#include "UDiagram.h"
#include "UBar.h"
#include "UBlock.h"
#include "FVector.h"
#include "Util.h"
#include "UInputManager.h"

class UBall : public UDiagram
{
public:
    FVector Location;           // 공의 위치
    FVector Velocity;           // 공의 방향
    float   Speed;                // 공의 속력
    float   Radius;               // 공의 반지름
    float   Mass;                 // 공의 질량 (반지름과 비례하다고 가정)
    bool    IsMove;
    UBar*   BarPtr;
    float   Acceleration;
    float   SpeedLimitMin;
    float   SpeedLimitMax;
    inline static int TotalNumBalls{ 0 };
private:
    std::deque<FVector> trailSpawnLoc;

    int maxTrailCount = 5;
    float trailTimer = 0.01f;
    float curTimer = 0.f;

    // 생성자 및 소멸자
public:
    UBall();

    UBall(const FVector& _Location, const FVector& _Velocity, const float _Speed, const float _Radius, const bool _IsMove, UBar* _BarPtr, const float _Acceleration, const float _SpeedLimit);

    virtual ~UBall() override;

    // UPrimitive 인터페이스 구현
    // 물리/이동 업데이트
    virtual void Update(float deltaTime);

    // 렌더링 (상수 버퍼 업데이트)
    virtual void Render(URenderer& renderer);

    // 벽 충돌 적용
    virtual void ApplyWallCollision();

    // 충격량 적용
    virtual void ApplyGravity(float deltaTime, const FVector& gravity);

    // 반지름 설정 (질량 자동 설정, 반지름에 비례)
    void SetRadius(float InRadius);

    // �� �ӵ� ����
    float GetSpeed();
    void SetSpeed(float inSpeed);

	virtual bool CheckCollision(const UDiagram* Other) override;

    EBlockCollision CheckBarCollision(const UBar& Bar, FVector& CollisionPos);

    void BallBounceAtBar(const EBlockCollision Position, const UBar& Bar, const FVector& CollisionPos);

    EBlockCollision CheckBlockCollision(const UBlock& Block, FVector& CollisionPos);

    void BallBounceAtBlock(const EBlockCollision Position, UBlock& Block, const FVector& CollisonPos);

    void ResolveCollision(UBall* Other);

    void SetIsMove(const bool _IsMove);

    bool GetIsMove();

    static UBall* CreateBallAtBar(const UBar& Bar);

    static UBall** CreateMultiBalls(const UBall* sourceBall);

    // static void InitBall(UBall& input);
};
