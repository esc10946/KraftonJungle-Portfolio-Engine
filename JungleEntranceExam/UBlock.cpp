#include "UBlock.h"

#include "UGameObject.h"
#include "URenderer.h"
#include <d3d11.h>
#include <cmath>
static int TotalScore=0;
UBlock::UBlock(EBlockType InType, EBlockColor InColor, int Round) :Type(InType),CenterX(0),CenterY(0),HalfW(0),HalfH(0)
{
    switch (Type)
    {
    case EBlockType::Normal:
        MaxHp = 1;
		Color = InColor;
        score = static_cast<int>(InColor);
        break;
    case EBlockType::Hard:
        MaxHp = 2 + (Round / 8);
        Color = EBlockColor::Silver;
        score = 50*Round;
        break;
    case EBlockType::Immortal:
        MaxHp = INT_MAX;
        Color = EBlockColor::Gold;
        score = 0;// temp Score
        break;
    default:
        MaxHp = 1;
		Color = EBlockColor::White;
        score = 1;
        break;
    }
    CurrHp = MaxHp;
   // SetTag("Block");
}

UBlock::~UBlock()
{
}

void UBlock::Init(float cx, float cy, float hw, float hh)
{
    CenterX = cx;
    CenterY = cy;
    HalfH = hh;
    HalfW = hw;
}
void UBlock::Update(float DeltaTime)
{
    // hmm...

}
bool UBlock::CheckBallCollision(const FVector& BallPos, float Radius, FVector& OutNormal) const
{
    if (!IsActive()) return false;
    const float overlapX = (HalfW + Radius) - fabsf(BallPos.x - CenterX);
    const float overlapY = (HalfH + Radius) - fabsf(BallPos.y - CenterY);

    if (overlapX <= 0.0f||overlapY<=0.0f)
        return false;

    if (overlapX < overlapY)
        OutNormal = FVector(BallPos.x < CenterX ? -1.0f : 1.0f, 0.0f, 0.0f);
    else
        OutNormal = FVector(0.0f, BallPos.y < CenterY ? -1.0f : 1.0f, 0.0f);

    return true;
}// 아마 사용안할수도있음 공쪽에서 콜리전 체크를 다하면 없앨예정

//void Block::Render(URenderer& Renderer)
//{
//    if (!IsActive())
//        return;
//    FColor color;
//    switch (Type)
//    {
//	case EBlockType::Normal:
//        color = GetColorFromBlockColor(Color);
//        break;
//    case EBlockType::Hard:
//        color = FColor(0.53f, 0.60f, 0.67f); 
//        break;
//	case EBlockType::Immortal:
//        color = FColor(1.00f, 0.55f, 0.10f);
//        break;
//    default:
//        color = FColor(1.0f, 1.0f, 1.0f);
//        break;
//            
//    }
//    Renderer.RenderRect(CenterX, CenterY, HalfW, HalfH, color);
//}


int UBlock::TakeDamage()//쿨타임 필요할거같은데혹은 다른 충돌이 있으면 초기화되던가
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

FColor UBlock::GetColor() const
{
	if (!IsActive())
		return FColor(0.0f, 0.0f, 0.0f, 0.0f);
    return GetColorFromBlockColor(Color);
}