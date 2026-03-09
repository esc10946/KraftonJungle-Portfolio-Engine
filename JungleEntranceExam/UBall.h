#pragma once

#include "UDiagram.h"
#include "FVector.h"
#include "FVertexSimple.h"

class UBall : public UDiagram
{
public:
    FVector Location;           // 공의 위치
    FVector Velocity;           // 공의 속도
    float Radius;               // 공의 반지름
    float Mass;                 // 공의 질량 (반지름과 비례하다고 가정)
    inline static int TotalNumBalls{ 0 };
    inline static ID3D11Buffer* SphereVertexBuffer{ nullptr };

    // 생성자 및 소멸자
public:
    static void Init_VertexBuffer(ID3D11Buffer* _SphereVertexBuffer);

    UBall();

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

	float GetRadius() const;

	const FVector& GetLocation() const;

	virtual bool CheckCollision(const UDiagram* Other) override;

    void ResolveCollision(UBall* Other);
};
