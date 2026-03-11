#include "UGameScene.h"
#include "UGameObject.h"
#include "UGameManager.h" 
#include "UBall.h"
#include "UBar.h"
#include "Util.h"
#include "UClearScene.h"
#include "USceneManager.h"
#include "USoundManager.h"
#include "UItemManager.h"
#include "Stage.h"
#include "UParticlePool.h"
#include "FileInfo.h"
UGameScene::UGameScene()
{
    Init(); 
}

UGameScene::~UGameScene()
{
    Release(); 
}

void UGameScene::UIRender()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    float padding = 20.0f;

    ImVec2 windowPos(viewport->WorkPos.x + viewport->WorkSize.x - padding, viewport->WorkPos.y + padding);

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGuiWindowFlags hudFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("HUD", nullptr, hudFlags);

    ImGui::SetWindowFontScale(4.0f);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "HIGH: %d", GetHightScore());
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "SCORE: %d", gameManager->GetTotalScore());
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "LIVES: %d", gameManager->GetCurLife());
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//?대떦 寃뚯엫?먯꽌 ?앹꽦?섎뒗 紐⑤뱺 ?ㅻ툕?앺듃?ш린???앹꽦
void UGameScene::Init()
{
    UGameObjectList.clear();
    SceneType = ESceneType::InGame;

    ActiveBallList.clear();

    //1踰??뚮젅?댁뼱媛 ?吏곸씠??諛?
    Bar_1 = new UBar(FVector(0.0f, -0.95f, 0.0f), 1.0f, 0.15f, 0, EPlaySide::Up);

    //2踰??뚮젅?댁뼱媛 ?吏곸씠??諛?
    Bar_2 = new UBar(FVector(0.0f, 0.95f, 0.0f), 1.0f, 0.15f, 1, EPlaySide::Down);

    UBall* newBall = UBall::CreateBallAtBar(*Bar_1);

    AddObject(newBall);
    AddObject(Bar_1);
    AddObject(Bar_2);


    int CurrentRound = 2;
    stageblocks = CreateStage(CurrentRound);
    for (auto& b : stageblocks)
    {
        if (!b)
            nullptr;
		AddObject(b);
    }



    GetStageInfo(CurrentStage, CurrentStageRow, CurrentStageCol);

    //寃뚯엫留ㅻ땲? 珥덇린??
    particlePool = new UParticlePool(200);
    gameManager = UGameManager::GetInstance();
    gameManager->RessetGM();
}

void UGameScene::Release()
{
    //Map?먯꽌 ?좊떦??brick?ㅼ쓣 ?댁젣?댁빞??

    //?앹꽦??紐⑤뱺 Ball???쒓굅
    for (UBall* ball : ActiveBallList)
    {
        if (ball != nullptr) {
            delete ball;
        }
    }
    ActiveBallList.clear();

    //?앹꽦??紐⑤뱺 UGameObject???쒓굅
    for (UGameObject* Object : UGameObjectList)
    {
        if (Object != nullptr) {
            delete Object;
        }
    }

    UGameObjectList.clear();
    if (particlePool != nullptr)
    {
        delete particlePool;
        particlePool = nullptr;
    }
    stageblocks.clear();
}

/// <summary>
/// ?대떦 留듭뿉 ?덈뒗 紐⑤뱺 媛앹껜???낅뜲?댄듃瑜??몄텧??
/// </summary>
/// <param name="delta"></param>
void UGameScene::Update(float delta)
{
    FVector CollisionPos;
    FVector Dummy;
    for (UGameObject* Object : UGameObjectList)
    {
        UDiagram* Diagram = dynamic_cast<UDiagram*>(Object);
        if (Diagram != nullptr) {
            Diagram->Update(delta);
        }
    }

    for (UBall* ball : ActiveBallList)
    {
        if (ball != nullptr) {
            ball->Update(delta);
        }

        EBlockCollision CollisionState1 = ball->CheckBarCollision(*Bar_1, CollisionPos);
        ball->BallBounceAtBar(CollisionState1, *Bar_1, CollisionPos);
        
        EBlockCollision CollisionState2 = ball->CheckBarCollision(*Bar_2, CollisionPos);
        ball->BallBounceAtBar(CollisionState2, *Bar_2, CollisionPos);
    

        int idx = -1;
        int CurRow = 0;
        int CurCol = 0;
        for (auto* b : stageblocks)
        {

            if (!b)
                continue;

            if (!b->IsActive()) 
                continue;

            idx++;
            if (!b || !b->IsActive()) continue;
            if (b->CheckSkip())
            {
                b->SetSkipCalc(false);
                continue;
            }


            EBlockCollision CollisionState = (*ball).CheckBlockCollision(*b, CollisionPos);
            if (CollisionState != EBlockCollision::None)
            {
                if (CollisionState == EBlockCollision::Corner)
                {
                    CurRow = idx / CurrentStageCol;
                    CurCol = idx % CurrentStageCol;

                    // ?꾩옱 異⑸룎??紐⑥꽌由?諛⑺뼢???곕Ⅸ ?몄젒 釉붾줉 議곗궗 諛⑺뼢 ?ㅼ젙
                    int dCol = (CollisionPos.x < b->CenterX) ? -1 : 1; // ?쇱そ 紐⑥꽌由щ㈃ -1, ?ㅻⅨ履쎌씠硫?1
                    int dRow = (CollisionPos.y < b->CenterY) ? 1 : -1; // ?꾨옒履?紐⑥꽌由щ㈃ 1, ?꾩そ?대㈃ -1 (醫뚰몴怨??뺤씤 ?꾩슂)

                    bool HasHorizontalBlock = false;
                    bool HasVerticalBlock = false;

                    // 1. ???섑룊) 諛⑺뼢??釉붾줉???덈뒗吏 ?뺤씤
                    int CheckCol = CurCol + dCol;
                    if (CheckCol >= 0 && CheckCol < CurrentStageCol) {
                        if (stageblocks[CurRow * CurrentStageCol + CheckCol]
                            && stageblocks[CurRow * CurrentStageCol + CheckCol]->IsActive())
                            HasHorizontalBlock = true;
                    }

                    // 2. ???꾨옒(?섏쭅) 諛⑺뼢??釉붾줉???덈뒗吏 ?뺤씤
                    int CheckRow = CurRow + dRow;
                    if (CheckRow >= 0 && CheckRow < CurrentStageRow) {
                        if (stageblocks[CheckRow * CurrentStageCol + CurCol]
                            && stageblocks[CheckRow * CurrentStageCol + CurCol]->IsActive())
                            HasVerticalBlock = true;
                    }

                    // 3. ?먯젙 援먯젙 濡쒖쭅
                    if (HasHorizontalBlock && !HasVerticalBlock) {
                        // ?놁? 留됲삍怨????꾨옒媛 鍮꾩뿀??-> ?쀫㈃/?꾨옯硫?異⑸룎濡?媛꾩＜
                        CollisionState = EBlockCollision::Horizontal;
                        stageblocks[CurRow * CurrentStageCol + CheckCol]->SetSkipCalc(true);
                    }
                    else if (!HasHorizontalBlock && HasVerticalBlock) {
                        // ???꾨옒??留됲삍怨??놁씠 鍮꾩뿀??-> ?녿㈃ 異⑸룎濡?媛꾩＜
                        CollisionState = EBlockCollision::Vertical;
                        stageblocks[CheckRow * CurrentStageCol + CurCol]->SetSkipCalc(true);
                    }
                    // ????鍮꾩뼱?덉쑝硫?else) ?먮옒??Corner ?먯젙 ?좎? (諛섏궗 踰≫꽣 ?ъ슜)
                }

                (*ball).BallBounceAtBlock(CollisionState, *b, CollisionPos);
                gameManager->SetScore(b->GetScore()); 
                USoundManager::GetInstance().Play("Brick");
            }
            
        }
    }

    // Item Objects Update
    UItemManager::Get().Update(delta);
    UItemManager::Get().CheckCollision(Bar_1);

    gameManager->Update(delta);

    // 諛뽰뿉 怨듭씠 ?섍컮?붿? ?먮퀎
    if (!HaveBalls())
    {
        // 怨듭씠 ???섍컮?쇰땲 ??怨듭쓣 ?섎굹 ?ㅽ룿?댁쨳?덈떎.
        UBall* newBall = UBall::CreateBallAtBar(*Bar_1);
        ActiveBallList.push_back(newBall);

        USoundManager::GetInstance().Play("Damage");
        gameManager->SubHealth(1);
    }
    particlePool->Update(delta);
    if (bIsBrickEmpty()) // 踰쎈룎 ??源⑥쭚!
    {
        // 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙占쌀쏙옙 占쏙옙占쏙옙
        UItemManager::Get().Clear();

        USoundManager::GetInstance().StopAll();
        USoundManager::GetInstance().Play("Victory");
        UClearScene::FinalScore = gameManager->GetTotalScore();
        HightScoreUpdate(UClearScene::FinalScore);
        USceneManager::GetInstance().LoadScene(ESceneType::Clear);
        return;
    }
}

/// <summary>
/// 紐⑤뱺 怨듭쓣 ?뺤씤?섍퀬 ?⑥? 寃??덈뒗吏 ?щ?瑜?諛섑솚
/// </summary>
/// <returns></returns>
bool UGameScene::HaveBalls()
{
    bool hasBallLeft = false;

    for (auto it = ActiveBallList.begin(); it != ActiveBallList.end(); )
    {
        UBall* ball = *it;

        //?덉쇅 泥섎━
        if (ball == nullptr) {
            it = ActiveBallList.erase(it);
            continue;
        }

        FVector Location = ball->Location;
        float Radius = ball->Radius;

        //留뚯빟??怨듭씠 諛뽰쑝濡??섍?吏 ?딆븯?쇰㈃ ?ㅼ쓬嫄??뺤씤
        if (Location.y < 1 - Radius && Location.y > -1 + Radius) {
            //怨듭씠 ?꾩쭅 ?⑥븘?덉쓬
            hasBallLeft = true;
            ++it; 
        }
        else {
            delete ball;
            it = ActiveBallList.erase(it);
        }
    }

    return hasBallLeft;
}

/// <summary>
/// 怨듬쭔 ?곕줈 援щ텇吏볥뒗 肄붾뱶 
/// </summary>
/// <param name="Object"></param>
void UGameScene::AddObject(UGameObject* Object)
{
    UBall* Ball = dynamic_cast<UBall*>(Object);

    if (Ball != nullptr) {
        ActiveBallList.push_back(Ball);
    }
    else {
        UGameObjectList.push_back(Object);
    }
}

bool UGameScene::bIsBrickEmpty()
{
    for (auto& b : stageblocks) {
        if (b && b->IsActive()) return false;
    }
    return true;
}

std::vector<UBall*>& UGameScene::GetActiveBalls()
{
    return ActiveBallList;
}

void UGameScene::AddBall(UBall* ball)
{
    ActiveBallList.push_back(ball);
}

void UGameScene::Render(URenderer render)
{
    for (UGameObject* Object : UGameObjectList)
    {
        UDiagram* Diagram = dynamic_cast<UDiagram*>(Object);

        if (Diagram != nullptr) {
            Diagram->Render(render);
            render.RenderRectangle();
        }
    }

    for (UBall* ball : ActiveBallList)
    {
        if (ball != nullptr) {
            ball->Render(render);
            render.RenderSphere();
        }
    }

    // Item Objects Render
    UItemManager::Get().Render(render);

    for (auto* b : stageblocks)
    {

        if (!b) //<-added

            continue;
        b->Render(render);
    }
    if (particlePool) {
        particlePool->Render(render);
    }
}

