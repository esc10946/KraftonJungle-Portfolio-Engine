#include "UBall.h"
#include "USoundManager.h"

// 생성자 및 소멸자
UBall::UBall() : 
    Location(0.0f, 0.0f, 0.0f), 
    Velocity(0.0f, 0.0f, 0.0f),
    Speed(1.0f), 
    Radius(0.1f), 
    Mass(0.1f), 
    IsMove(false), 
    BarPtr(nullptr), 
    Acceleration(0.0f),
    SpeedLimitMin(0.5f),
    SpeedLimitMax(5.0f)
{
    ++TotalNumBalls;
    Mass = (4.0f / 3.0f) * Pi * std::powf(Radius, 3);
}

UBall::UBall(const FVector& _Location, const FVector& _Velocity,const float _Speed, const float _Radius, const bool _IsMove, UBar* _BarPtr, const float _Acceleration, const float _SpeedLimit)
    : Location(_Location), Velocity(_Velocity),Speed(_Speed), Radius(_Radius), IsMove(_IsMove), BarPtr(_BarPtr), Acceleration(_Acceleration), SpeedLimitMax(_SpeedLimit)
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
    static int StartMoveKey = (BarPtr->PlayerNo == 0) ? VK_UP : 'S';

    if (IsMove)
    {
        // 속도에 기반하여 위치 적용
        Location.x += Velocity.x * Speed * deltaTime;
        Location.y += Velocity.y * Speed * deltaTime;
        // Location.z += Velocity.z * deltaTime;

        // 벽 충돌 적용
        ApplyWallCollision();

        if (Speed < SpeedLimitMax)
            Speed += Acceleration;
    }
    else
    {
        if (BarPtr != nullptr)
        {
            Location = BarPtr->Location + FVector(0, Radius * static_cast<int>(BarPtr->Side), 0);
        }
        UInputManager* input = UInputManager::GetInstance();

        if (input->GetKeyDown(StartMoveKey))
        {
            IsMove = true;
        }
    }
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

float UBall::GetSpeed()
{
    return Speed;
}

void UBall::SetSpeed(float inSpeed)
{
    Speed = inSpeed;

    if (Speed > SpeedLimitMax) Speed = SpeedLimitMax;
    else if (Speed < SpeedLimitMin) Speed = SpeedLimitMin;
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
		float DistanceSquared{ dx * dx + dy * dy };
		float radiusSum{ Radius + OtherBall->Radius };
		return DistanceSquared <= (radiusSum * radiusSum);
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
        bool HitHorizontal = (Bar.Location.x - Bar.XLength <= Location.x && Location.x <= Bar.Location.x + Bar.XLength);
        bool HitVertical = (Bar.Location.y - Bar.YLength <= Location.y && Location.y <= Bar.Location.y + Bar.YLength);

        CollisionPos.x = ClosestX;
        CollisionPos.y = ClosestY;

        if (HitVertical)
            return EBlockCollision::Vertical;
        else if (HitHorizontal)
            return EBlockCollision::Horizontal;
        else
            return EBlockCollision::Corner;
    }
    return EBlockCollision::None;
}

void UBall::BallBounceAtBar(const EBlockCollision Position, const UBar& Bar, const FVector& CollisionPos)
{
    if (Position == EBlockCollision::None) return;

    if(IsMove) USoundManager::GetInstance().Play("Hit");

    switch (Position)
    {
    case EBlockCollision::Horizontal:
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
    case EBlockCollision::Vertical:
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
        int YSign = Velocity.y > 0 ? 1 : -1;

        if (Dist < 1e-6f) Dist = 1.0f;

        FVector NormalizedNormal = NormalDir / Dist;

        float DotVN = Velocity.Dot(NormalizedNormal);
        if (DotVN < 0.0f) // 공이 면을 향해 다가올 때만
        {
            FVector R = Velocity - (NormalizedNormal * 2.0f * DotVN);
            Velocity = R;
        }
        if (std::abs(Velocity.y) < 0.3f)
        {
            Velocity.y = (Velocity.y >= 0.0f) ? 0.3f : -0.3f;
            Velocity.x = std::sqrt(1.0f - Velocity.y * Velocity.y) * ((Velocity.x >= 0.0f) ? 1.0f : -1.0f);
        }
        Velocity.y *= (Velocity.y * YSign > 0 ? -1 : 1);

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
    float DistanceX = Location.x - ClosestX;
    float DistanceY = Location.y - ClosestY;
    float DistanceSquared = (DistanceX * DistanceX) + (DistanceY * DistanceY);

    if (DistanceSquared < (Radius * Radius))
    {
        // 충돌 발생! 어느 면인지 판정
        bool HitVertical = (Block.MinY <= Location.y && Location.y <= Block.MaxY);
        bool HitHorizontal = (Block.MinX <= Location.x && Location.x <= Block.MaxX);

        CollisionPos.x = ClosestX;
        CollisionPos.y = ClosestY;
        if (HitVertical)
            return EBlockCollision::Vertical;
        else if (HitHorizontal)
            return EBlockCollision::Horizontal;
        else
            return EBlockCollision::Corner;
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
            // 좌측 또는 우측 면 충돌
            Velocity.x *= -1.0f;
            Location.x = (Location.x > Block.CenterX) ? Block.MaxX + Radius : Block.MinX - Radius;
            break;
        }
        case EBlockCollision::Horizontal:
        {
            // 상단 또는 하단 면 충돌
            Velocity.y *= -1.0f;
            Location.y = (Location.y > Block.CenterY) ? Block.MaxY + Radius : Block.MinY - Radius;
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

            if (std::abs(Velocity.y) < 0.3f)
            {
                Velocity.y = (Velocity.y >= 0.0f) ? 0.3f : -0.3f;
                Velocity.x = std::sqrt(1.0f - Velocity.y * Velocity.y) * ((Velocity.x >= 0.0f) ? 1.0f : -1.0f);
            }
            Location = CollisionPos + (NormalizedNormal * (Radius + 0.001f));

            break;
        }
    }
}

void UBall::SetIsMove(const bool _IsMove)
{
    IsMove = _IsMove;
}

bool UBall::GetIsMove()
{
    return IsMove;
}

UBall* UBall::CreateBallAtBar(const UBar& Bar)
{
    // new 연산자를 사용해 공의 Instance를 생성
    UBall* Ball = new UBall();

    float maxRadiusX = (rightBorder - leftBorder) * 0.05f;
    float maxRadiusY = (topBorder - bottomBorder) * 0.05f;
    float maxAllowedRadius = (maxRadiusX < maxRadiusY) ? maxRadiusX : maxRadiusY;
    float r = maxAllowedRadius / 2;
    Ball->SetRadius(r);

    Ball->Location.x = Bar.Location.x;
    Ball->Location.y = Bar.Location.y + (Bar.YLength + Ball->Radius) * static_cast<int>(Bar.Side);
    Ball->Location.z = 0.0f;

    Ball->Velocity.y = GetRandomFloat(0.8f, 1.0f);
    Ball->Velocity.x = sqrt(1 - Ball->Velocity.y * Ball->Velocity.y) * GetRandomSide();
    Ball->Velocity.z = 0.0f;
    Ball->Speed = 0.5f;

    Ball->SetIsMove(false);
    Ball->BarPtr = const_cast<UBar*>(&Bar);
    Ball->Acceleration = 0.0002f;

    return Ball;
}

UBall** UBall::CreateMultiBalls(const UBall* sourceBall)
{
    if (sourceBall == nullptr)
        return nullptr;

    UBall** createdBalls = new UBall * [2];

    createdBalls[0] = new UBall();
    createdBalls[1] = new UBall();

    // 원본 공 속성 복사
    createdBalls[0]->SetRadius(sourceBall->Radius);
    createdBalls[1]->SetRadius(sourceBall->Radius);

    createdBalls[0]->Location = sourceBall->Location;
    createdBalls[1]->Location = sourceBall->Location;

    createdBalls[0]->Speed = sourceBall->Speed;
    createdBalls[1]->Speed = sourceBall->Speed;

    createdBalls[0]->Acceleration = sourceBall->Acceleration;
    createdBalls[1]->Acceleration = sourceBall->Acceleration;

    // 대각선 방향 벡터 정규화
    const float diagonal = 0.70710678f; // 1 / sqrt(2)

    // 왼쪽 위 대각선
    createdBalls[0]->Velocity.x = -diagonal * sourceBall->Speed;
    createdBalls[0]->Velocity.y = diagonal * sourceBall->Speed;
    createdBalls[0]->Velocity.z = 0.0f;

    // 오른쪽 위 대각선
    createdBalls[1]->Velocity.x = diagonal * sourceBall->Speed;
    createdBalls[1]->Velocity.y = diagonal * sourceBall->Speed;
    createdBalls[1]->Velocity.z = 0.0f;

    // 겹침 방지용으로 위치 약간 분리
    createdBalls[0]->Location.x -= sourceBall->Radius * 0.5f;
    createdBalls[1]->Location.x += sourceBall->Radius * 0.5f;

    createdBalls[0]->SetIsMove(true);
    createdBalls[1]->SetIsMove(true);

    return createdBalls;
}

//void UBall::InitBall(UBall& input)
//{
//    // 임의의 크기(Radius): 너무 큰 값을 방지하기 위해, 공의 크기를 화면 너비의 1/10로 제한
//    float maxRadiusX = (rightBorder - leftBorder) * 0.05f;
//    float maxRadiusY = (topBorder - bottomBorder) * 0.05f;
//    float maxAllowedRadius = (maxRadiusX < maxRadiusY) ? maxRadiusX : maxRadiusY;
//    float r = 0.05f; //GetRandomFloat(0.1f, 0.2f);
//    input.SetRadius(r);
//
//    // 임의의 위치(Location): 화면 경계 안쪽의 랜덤한 위치, 반지름을 마진값으로 함
//    input.Location.x = GetRandomFloat(leftBorder + input.Radius, rightBorder - input.Radius);
//    input.Location.y = GetRandomFloat(bottomBorder + input.Radius, topBorder - input.Radius);
//    input.Location.z = 0.0f;
//
//    // 임의의 속도(Velocity)
//    input.Velocity.x = 0.0f; //GetRandomFloat(1.5f, 2.0f);
//    input.Velocity.y = -1.0f; //GetRandomFloat(1.5f, 2.0f);
//    input.Velocity.z = 0.0f;
//
//    input.Speed = 0.3f;
//}