#include "UBlock.h"
#include "UItemManager.h"
#include "ItemLibrary.h"
#include "UGameScene.h"
#include "USceneManager.h"
#include "UParticlePool.h"
#include "UParticle.h"
static int TotalScore=0;// ?꾩옱 ?꾩껜 ?ㅼ퐫??

static int TotalActiveBlocks = 0;// ?꾩옱 ?쒖꽦?붾맂 釉붾줉 ??

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
        WipeProgress += DeltaTime * 12.0f; // 踰붿쐞媛 ?볦뼱議뚯쑝???띾룄瑜?議곌툑 ?щ┝

        if (WipeProgress >= 2.5f)
        {
            WipeProgress = -3.0f; // ?꾩쟾??爰쇱쭊 ?곹깭
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



int UBlock::TakeDamage()
{
    if (!IsActive() || Type == EBlockType::Immortal)
        return 0;
    CurrHp--;

    if (CurrHp == 0)
        return BreakBlock();

    else
        WipeProgress = -2.5f;
    return 0;
}
int UBlock::BreakBlock()
{
    FColor blockColor = GetColor(); 
    SetActive(false);
    TotalScore += score;
    TotalActiveBlocks--;

    FItemDesc ItemDesc = ItemLibrary::MakeRandomItem();
    UItemManager::Get().SpawnItem(ItemDesc, FVector(CenterX, CenterY, 0.0f), FVector(0.0f, -1.0f, 0.0f));
    for (int i = 0; i < 15; ++i) {
        UParticle* p = dynamic_cast<UGameScene*>(USceneManager::GetInstance().GetCurrentScene())->GetParticlePool()->GetInactiveParticle();
        if (p) {
            FVector randVel(((rand() % 200) - 100) / 100.0f, ((rand() % 200) - 100) / 100.0f, 0);
            p->Spawn(FVector(CenterX, CenterY, 0), randVel, blockColor, 2.0f);
        }
    }

    return score;
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
