#include "UBlock.h"

static int TotalScore = 0;

UBlock::UBlock(EBlockType InType, EBlockColor InColor, int Round) :Type(InType), CenterX(0), CenterY(0), HalfW(0), HalfH(0), MinX(0), MaxX(0), MinY(0), MaxY(0), Color(InColor)
{
    switch (Type)
    {
    case EBlockType::Normal:   MaxHp = 1;                break;
    case EBlockType::Hard:     MaxHp = 2 + (Round / 8); break;
    case EBlockType::Immortal: MaxHp = INT_MAX;          break;
    }
    CurrHp = MaxHp;
    // SetTag("UBlock");
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
    MinX = CenterX - HalfW;
    MaxX = CenterX + HalfW;
    MinY = CenterY - HalfH;
    MaxY = CenterY + HalfH;
}
void UBlock::Update(float DeltaTime)
{
    // hmm...

}
bool UBlock::CheckBallCollision(const FVector& BallPos, float Radius, FVector& OutNormal) const
{
    //if (!IsActive()) return false;
    const float overlapX = (HalfW + Radius) - fabsf(BallPos.x - CenterX);
    const float overlapY = (HalfH + Radius) - fabsf(BallPos.y - CenterY);

    if (overlapX <= 0.0f || overlapY <= 0.0f)
        return false;

    if (overlapX < overlapY)
        OutNormal = FVector(BallPos.x < CenterX ? -1.0f : 1.0f, 0.0f, 0.0f);
    else
        OutNormal = FVector(0.0f, BallPos.y < CenterY ? -1.0f : 1.0f, 0.0f);

    return true;
}

void UBlock::Render(URenderer& Renderer)
{
    /*if (!IsActive())
        return;*/
    FColor color;
    switch (Type)
    {
    case EBlockType::Normal:
        color = GetColorFromBlockColor(Color);
        break;
    case EBlockType::Hard:
        color = FColor(0.53f, 0.60f, 0.67f);
        break;
    case EBlockType::Immortal:
        color = FColor(1.00f, 0.55f, 0.10f);
        break;
    default:
        color = FColor(1.0f, 1.0f, 1.0f);
        break;

    }
    Renderer.RenderRect(CenterX, CenterY, HalfW, HalfH, color);
}


int UBlock::TakeDamage()
{
    /*if (!IsActive() || Type == EBlockType::Immortal)
        return 0;*/
    CurrHp--;

    if (CurrHp == 0)
    {
        //SetActive(false);
        const int score = static_cast<int>(Color);
        TotalScore += score;
        return score;
    }
    return 0;
}
