#include "UBall.h"

// 생성자 및 소멸자
UBall::UBall() : Location(0.0f, 0.0f, 0.0f), Velocity(0.0f, 0.0f, 0.0f), Radius(0.1f), Mass(0.1f)
{
    ++TotalNumBalls;
    SetRadius(Radius);
}

UBall::~UBall()
{
    --TotalNumBalls;
}

    // UPrimitive 인터페이스 구현
    // 물리/이동 업데이트
void UBall::Update(float deltaTime)
{
    // 속도에 기반하여 위치 적용
    Location.x += Velocity.x * deltaTime;
    Location.y += Velocity.y * deltaTime;
    Location.z += Velocity.z * deltaTime;

    // 벽 충돌 적용
    ApplyWallCollision();
}

    // 렌더링 (상수 버퍼 업데이트)
void UBall::Render(URenderer& renderer)
{
    renderer.UpdateConstant(Location, Radius);
}

    // 벽 충돌 적용
void UBall::ApplyWallCollision()
{
    // 벽과 충돌 여부를 체크하고 반사 시킴 (벽 끼임 방지 추가)
    if (Location.x < leftBorder + Radius)
    {
        Location.x = leftBorder + Radius;
        if (Velocity.x < 0.0f) Velocity.x *= -1.0f;
    }
    if (Location.x > rightBorder - Radius)
    {
        Location.x = rightBorder - Radius;
        if (Velocity.x > 0.0f) Velocity.x *= -1.0f;
    }
    if (Location.y > topBorder - Radius)
    {
        Location.y = topBorder - Radius;
        if (Velocity.y > 0.0f) Velocity.y *= -1.0f;
    }
    if (Location.y < bottomBorder + Radius)
    {
        Location.y = bottomBorder + Radius;
        if (Velocity.y < 0.0f) Velocity.y *= -1.0f;
    }
}

    // 충격량 적용
void UBall::ApplyGravity(float deltaTime, const FVector& gravity)
{
    Velocity = Add(Velocity, Mul(gravity, deltaTime));
}

// 반지름 설정 (질량 자동 설정, 반지름에 비례)
void UBall::SetRadius(float InRadius)
{
    Radius = InRadius;

    Mass = Radius;
}

float UBall::GetRadius() const 
{ 
	return Radius;
}
const FVector& UBall::GetLocation() const
{ 
	return Location; 
}
void UBall::Init_VertexBuffer(ID3D11Buffer* _SphereVertexBuffer) 
{
	SphereVertexBuffer = _SphereVertexBuffer;
}

bool UBall::CheckCollision(const UDiagram* Other) {
	const UBall* OtherBall{ dynamic_cast<const UBall*>(Other) };
	if (OtherBall) {
		float dx{ Location.x - OtherBall->Location.x };
		float dy{ Location.y - OtherBall->Location.y };
		float distanceSquared{ dx * dx + dy * dy };
		float radiusSum{ Radius + OtherBall->Radius };
		return distanceSquared <= (radiusSum * radiusSum);
	}
	return false;
}

void UBall::ResolveCollision(UBall* Other) {
	float dx{ Other->Location.x - Location.x };
	float dy{ Other->Location.y - Location.y };
	float Distance{ sqrtf(dx * dx + dy * dy) };

	if (Distance == 0.0f) return;

	float nx{ dx / Distance };
	float ny{ dy / Distance };

	float rvx{ Other->Velocity.x - Velocity.x };
	float rvy{ Other->Velocity.y - Velocity.y };

	float VelocityAlongNormal{ rvx * nx + rvy * ny };

	if (VelocityAlongNormal > 0) return;

	float e{ 1.0f };

	float j{ -(1.0f + e) * VelocityAlongNormal };
	j /= (1.0f / Mass + 1.0f / Other->Mass);

	float ImpulseX{ j * nx };
	float ImpulseY{ j * ny };

	Velocity.x -= (1.0f / Mass) * ImpulseX;
	Velocity.y -= (1.0f / Mass) * ImpulseY;
	Other->Velocity.x += (1.0f / Other->Mass) * ImpulseX;
	Other->Velocity.y += (1.0f / Other->Mass) * ImpulseY;

	float Overlap{ (Radius + Other->Radius) - Distance };
	if (Overlap > 0) {
		float SeparationX{ (Overlap / 2.0f) * nx };
		float SeparationY{ (Overlap / 2.0f) * ny };
		Location.x -= SeparationX;
		Location.y -= SeparationY;
		Other->Location.x += SeparationX;
		Other->Location.y += SeparationY;
	}
}