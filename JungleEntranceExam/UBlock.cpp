#include "UBlock.h"
#include "UItemManager.h"
#include "ItemLibrary.h"
#include <iostream>

static int TotalScore=0;// 현재 전체 스코어

static int TotalActiveBlocks = 0;// 현재 활성화된 블록 수
UBlock::UBlock(EBlockType InType, EBlockColor InColor, int Round) :Type(InType),CenterX(0),CenterY(0),HalfW(0),HalfH(0),MaxX(0),MaxY(0),MinX(0),MinY(0)
{
   
    switch (Type)
    {
    case EBlockType::Normal:
        MaxHp = 1;
		Color = InColor;
        score = static_cast<int>(InColor);
		
        TotalActiveBlocks++;
        break;
    case EBlockType::Hard:
        MaxHp = 2 + (Round / 8);
        Color = EBlockColor::Silver;
        score = 50*Round;
        TotalActiveBlocks++;
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
    MinX = CenterX - HalfW;
    MaxX = CenterX + HalfW;
    MinY = CenterY - HalfH;
    MaxY = CenterY + HalfH;

}
void UBlock::Update(float DeltaTime)
{
    if (WipeProgress >= -2.5f && WipeProgress < 2.5f)
    {
        WipeProgress += DeltaTime * 12.0f; // 범위가 넓어졌으니 속도를 조금 올림

        if (WipeProgress >= 2.5f)
        {
            WipeProgress = -3.0f; // 완전히 꺼진 상태
        }
    }

}
void UBlock::Render(URenderer& Renderer)
{
    if (!IsActive())
        return;
    Renderer.UpdateConstant(
        FVector(CenterX, CenterY, 0.0f),
        FVector(HalfW, HalfH, 1.0f),
        GetColor(),
        GetWipeProgress()
    );
    Renderer.RenderRectangle();
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



int UBlock::TakeDamage()//쿨타임 필요할거같은데혹은 다른 충돌이 있으면 초기화되던가
{
    if (!IsActive() || Type == EBlockType::Immortal)
        return 0;
    CurrHp--;

    if (CurrHp == 0)
    {
        SetActive(false);
        TotalScore += score;
        TotalActiveBlocks--;

        // 아이템 생성
        FItemDesc ItemDesc = ItemLibrary::MakeRandomItem();

        UItemManager::Get().SpawnItem(ItemDesc, FVector(CenterX, CenterY, 0.0f), FVector(0.0f, -1.0f, 0.0f));

		std::cout << "Score : " << TotalScore << " Active Blocks : " << TotalActiveBlocks << std::endl;
        return score;
    }
    else
        WipeProgress = -2.5f;
    return 0;
}
int UBlock::GetScore()
{
	return TotalScore;
}

FColor UBlock::GetColor() const
{
	if (!IsActive())
		return FColor(0.0f, 0.0f, 0.0f, 0.0f);
    return GetColorFromBlockColor(Color);
}

float  UBlock::GetWipeProgress() const
{
	return WipeProgress;
}
