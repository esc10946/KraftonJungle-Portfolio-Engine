#include "Block.h"

#include "UGameObject.h"
#include "Shape.h"
#include <d3d11.h>
#include <cmath>
static int TotalScore=0;
Block::Block(EBlockType InType, int Round):Type(InType),CenterX(0),CenterY(0),HalfW(0),HalfH(0)
{
    switch (Type)
    {
    case EBlockType::Normal:   MaxHp = 1;                break;
    case EBlockType::Hard:     MaxHp = 2 + (Round / 8); break;
    case EBlockType::Immortal: MaxHp = INT_MAX;          break;
    }
    CurrHp = MaxHp;
   // SetTag("Block");
}

Block::~Block()
{
}

void Block::Init(float cx, float cy, float hw, float hh)
{
    CenterX = cx;
    CenterY = cy;
    HalfH = hh;
    HalfW = hw;
}
void Block::Update(float DeltaTime)
{
    // hmm...

}
bool Block::CheckBallCollision(const FVector& BallPos, float Radius, FVector& OutNormal) const
{
    if (!IsActive()) return false;
    const float overlapX = (HalfW + Radius) - fabsf(BallPos.x - CenterX);
    const float overlapY = (HalfH + Radius) - fabsf(BallPos.y - CenterY);

	return false;
}

void Block::Render(URenderer& Renderer)
{
    if (!IsActive()) return;
    FColor rgb;
    
}


int Block::TakeDamage()
{
    if (!IsActive() || Type == EBlockType::Immortal)
        return 0;
    CurrHp--;

    if (CurrHp == 0)
    {
        SetActive(false);
        const int score = static_cast<int>(Color);
        TotalScore += score;
        return score;
    }
	return 0;
}
