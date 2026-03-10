#include "UBall.h"
#include "USoundManager.h"

// 생성자 및 소멸자
UBall::UBall() : Location(0.0f, 0.0f, 0.0f), Velocity(0.0f, 0.0f, 0.0f),Speed(1.0f), Radius(0.1f), Mass(0.1f)
{
    ++TotalNumBalls;
    Mass = (4.0f / 3.0f) * Pi * std::powf(Radius, 3);
}

UBall::UBall(const FVector& _Location, const FVector& _Velocity,const float _Speed, const float _Radius) 
    : Location(_Location), Velocity(_Velocity),Speed(_Speed), Radius(_Radius)
{
    ++TotalNumBalls;
    Mass = (4.0f / 3.0f) * Pi * std::powf(Radius, 3);
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
    Location.x += Velocity.x * Speed * deltaTime;
    Location.y += Velocity.y * Speed * deltaTime;
    // Location.z += Velocity.z * deltaTime;

    // 벽 충돌 적용
    ApplyWallCollision();
}

    // 렌더링 (상수 버퍼 업데이트)
void UBall::Render(URenderer& renderer)
{
    renderer.UpdateConstant(Location, FVector(Radius, Radius, 0));
}

    // 벽 충돌 적용
void UBall::ApplyWallCollision()
{
    bool bHitWall = false;

    // 벽과 충돌 여부를 체크하고 반사 시킴 (벽 끼임 방지 추가)
    if (Location.x < leftBorder + Radius)
    {
        Location.x = leftBorder + Radius;
        if (Velocity.x < 0.0f)
        {
            Velocity.x *= -1.0f;
            bHitWall = true; // 부딪힘 체크!
        }
    }
    if (Location.x > rightBorder - Radius)
    {
        Location.x = rightBorder - Radius;
        if (Velocity.x > 0.0f)
        {
            Velocity.x *= -1.0f;
            bHitWall = true;
        }
    }
    if (Location.y > topBorder - Radius)
    {
        Location.y = topBorder - Radius;
        if (Velocity.y > 0.0f)
        {
            Velocity.y *= -1.0f;
        }
    }

    if (bHitWall)
    {
        USoundManager::GetInstance().Play("Hit");
    }
}

    // 충격량 적용
void UBall::ApplyGravity(float deltaTime, const FVector& gravity)
{
    //Velocity = Velocity + gravity * deltaTime;
}

// 반지름 설정 (질량 자동 설정, 반지름에 비례)
void UBall::SetRadius(float InRadius)
{
    Radius = InRadius;

    Mass = Mass = (4.0f / 3.0f) * Pi * std::powf(Radius, 3);
}

bool UBall::CheckCollision(const UDiagram* Other)
{
    const UBar* PlayerBar{ dynamic_cast<const UBar*>(Other) };
    if (PlayerBar)
    {
        if (Location.y - Radius <= PlayerBar->Location.y + PlayerBar->YLength)
        {
            return (PlayerBar->Location.x - (PlayerBar->XLength + Radius) <= Location.x && Location.x <= PlayerBar->Location.x + (PlayerBar->XLength + Radius));
        }
    }
	const UBall* OtherBall{ dynamic_cast<const UBall*>(Other) };
	if (OtherBall)
    {
		float dx{ Location.x - OtherBall->Location.x };
		float dy{ Location.y - OtherBall->Location.y };
		float distanceSquared{ dx * dx + dy * dy };
		float radiusSum{ Radius + OtherBall->Radius };
		return distanceSquared <= (radiusSum * radiusSum);
	}
	return false;
}

//void UBall::BallBounceAtBar(const UBar& PlayerBar)
//{
//    Velocity.x = Velocity.x + ((Location.x - PlayerBar.Location.x) / PlayerBar.XLength) / 3;
//    if (Velocity.x > 0.80f)
//        Velocity.x = 0.80f;
//    else if (Velocity.x < -0.80f)
//        Velocity.x = -0.80f;
//    Velocity.y = sqrtf(1 - powf(Velocity.x, 2));
//    Location.y = PlayerBar.Location.y + PlayerBar.YLength + Radius;
//}

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

EBlockCollision UBall::CheckBarCollision(const UBar& Bar, FVector& CollisionPos)
{
    // 2. 공의 중심에서 벽돌 위의 가장 가까운 점(P) 찾기
    float ClosestX = std::clamp(Location.x, Bar.Location.x - Bar.XLength, Bar.Location.x + Bar.XLength);
    float ClosestY = std::clamp(Location.y, Bar.Location.y - Bar.YLength, Bar.Location.y + Bar.YLength);

    // 3. 공의 중심과 점 P 사이의 거리 계산
    float DistanceX = Location.x - ClosestX;
    float DistanceY = Location.y - ClosestY;
    float DistanceSquared = (DistanceX * DistanceX) + (DistanceY * DistanceY);

    if (DistanceSquared < (Radius * Radius))
    {
        // 충돌 발생! 어느 면인지 판정
        bool HitVertical = (Location.x >= Bar.Location.x - Bar.XLength && Location.x <= Bar.Location.x + Bar.XLength);
        bool HitHorizontal = (Location.y >= Bar.Location.y - Bar.YLength && Location.y <= Bar.Location.y + Bar.YLength);

        if (HitVertical)
        {
            CollisionPos.x = (Location.x > Bar.Location.x) ? Location.x - Radius : Location.x + Radius;
            CollisionPos.y = Location.y;
            return EBlockCollision::Vertical;
        }
        else if (HitHorizontal)
        {
            CollisionPos.x = Location.x;
            CollisionPos.y = (Location.y > Bar.Location.y) ? Location.y - Radius : Location.y + Radius;
            return EBlockCollision::Horizontal;
        }
        else
        {
            CollisionPos.x = ClosestX;
            CollisionPos.y = ClosestY;
            return EBlockCollision::Corner;
        }
    }
    return EBlockCollision::None;
}

void UBall::BallBounceAtBar(const EBlockCollision Position, const UBar& Bar, const FVector& CollisionPos)
{
    if (Position == EBlockCollision::None) return;

    USoundManager::GetInstance().Play("Hit");

    switch (Position)
    {
    case EBlockCollision::Vertical:
    {
        // 상단 또는 하단 면 충돌
        Velocity.x = Velocity.x + ((Location.x - Bar.Location.x) / Bar.XLength) / 3;
            if (Velocity.x > 0.80f)
                Velocity.x = 0.80f;
            else if (Velocity.x < -0.80f)
                Velocity.x = -0.80f;
            Velocity.y = sqrtf(1 - powf(Velocity.x, 2)) * (Velocity.y < 0 ? 1.0f : -1.0f);
        Location.y = (Location.y > Bar.Location.y) ? Bar.Location.y + Bar.YLength + Radius : Bar.Location.y - Bar.YLength - Radius;
        break;
    }
    case EBlockCollision::Horizontal:
    {
        // 좌측 또는 우측 면 충돌
        Velocity.x *= -1.0f;
        Location.x = (Location.x > Bar.Location.x) ? Bar.Location.x + Bar.XLength + Radius : Bar.Location.x - Bar.XLength - Radius;
        break;
    }
    case EBlockCollision::Corner:
    {
        // 모서리 충돌
        FVector NormalDir = Location - CollisionPos;
        float Dist = NormalDir.Length();

        if (Dist < 1e-6f) Dist = 1.0f;

        FVector NormalizedNormal = NormalDir / Dist;

        float DotVN = Velocity.Dot(NormalizedNormal);
        if (DotVN < 0.0f) // 공이 면을 향해 다가올 때만
        {
            FVector R = Velocity - (NormalizedNormal * 2.0f * DotVN);
            Velocity = R;
        }

        if (std::abs(Velocity.y) < 0.2f)
        {
            Velocity.y = (Velocity.y >= 0.0f) ? 0.2f : -0.2f;
            float xSign = (Velocity.x >= 0.0f) ? 1.0f : -1.0f;
            Velocity.x = std::sqrt(1.0f - Velocity.y * Velocity.y) * xSign;
        }
        if (Velocity.y < 0.0f)
        {
            Velocity.y *= -1;
        }
        Location = CollisionPos + (NormalizedNormal * (Radius + 0.001f));

        break;
    }
    }
}

EBlockCollision UBall::CheckBlockCollision(const UBlock& Block, FVector& CollisionPos)
{
    // 2. 공의 중심에서 벽돌 위의 가장 가까운 점(P) 찾기
    float ClosestX = std::clamp(Location.x, Block.MinX, Block.MaxX);
    float ClosestY = std::clamp(Location.y, Block.MinY, Block.MaxY);

    // 3. 공의 중심과 점 P 사이의 거리 계산
    float distanceX = Location.x - ClosestX;
    float distanceY = Location.y - ClosestY;
    float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);

    if (distanceSquared < (Radius * Radius))
    {
        // 충돌 발생! 어느 면인지 판정
        bool HitVertical = (Location.x >= Block.MinX && Location.x <= Block.MaxX);
        bool HitHorizontal = (Location.y >= Block.MinY && Location.y <= Block.MaxY);

        if (HitVertical)
        {
            CollisionPos.x = (Location.x > Block.CenterX) ? Location.x - Radius : Location.x + Radius;
            CollisionPos.y = Location.y;
            return EBlockCollision::Vertical;
        }
        else if (HitHorizontal)
        {
            CollisionPos.x = Location.x;
            CollisionPos.y = (Location.y > Block.CenterY) ? Location.y - Radius : Location.y + Radius;
            return EBlockCollision::Horizontal;
        }
        else
        {
            CollisionPos.x = ClosestX;
            CollisionPos.y = ClosestY;
            return EBlockCollision::Corner;
        }
    }
    return EBlockCollision::None;
}

void UBall::BallBounceAtBlock(const EBlockCollision Position, UBlock& Block, const FVector& CollisionPos)
{
    if (Position == EBlockCollision::None) return;

    Block.TakeDamage();

    switch (Position)
    {
        case EBlockCollision::Vertical:
        {
            // 상단 또는 하단 면 충돌
            Velocity.y *= -1.0f;
            Location.y = (Location.y > Block.CenterY) ? Block.MaxY + Radius : Block.MinY - Radius;
            break;
        }
        case EBlockCollision::Horizontal:
        {
            // 좌측 또는 우측 면 충돌
            Velocity.x *= -1.0f;
            Location.x = (Location.x > Block.CenterX) ? Block.MaxX + Radius : Block.MinX - Radius;
            break;
        }
        case EBlockCollision::Corner:
        {
            // 모서리 충돌
            FVector NormalDir = Location - CollisionPos;
            float Dist = NormalDir.Length();

            if (Dist < 1e-6f) Dist = 1.0f;

            FVector NormalizedNormal = NormalDir / Dist;

            float DotVN = Velocity.Dot(NormalizedNormal);
            if (DotVN < 0.0f) // 공이 면을 향해 다가올 때만
            {
                FVector R = Velocity - (NormalizedNormal * 2.0f * DotVN);
                Velocity = R;
            }

            if (std::abs(Velocity.y) < 0.2f)
            {
                Velocity.y = (Velocity.y >= 0.0f) ? 0.2f : -0.2f;
                float xSign = (Velocity.x >= 0.0f) ? 1.0f : -1.0f;
                Velocity.x = std::sqrt(1.0f - Velocity.y * Velocity.y) * xSign;
            }
            Location = CollisionPos + (NormalizedNormal * (Radius + 0.001f));

            break;
        }
    }
}

