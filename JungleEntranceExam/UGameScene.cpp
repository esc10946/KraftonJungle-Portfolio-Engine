#include "UGameScene.h"
#include "UGameObject.h"
#include "UGameManager.h" 
#include "UBall.h"
#include "UBar.h"
#include "Util.h"
#include "UClearScene.h"
#include "USceneManager.h"
#include "USoundManager.h"
#include "Stage.h"

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
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "SCORE: %d", gameManager->GetTotalScore());
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "LIVES: %d", gameManager->GetCurLife());
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//해당 게임에서 생성되는 모든 오브젝트여기서 생성
void UGameScene::Init()
{
    UGameObjectList.clear();
    SceneType = ESceneType::InGame;

    ActiveBallList.clear();

    //1번 플레이어가 움직이는 바
    Bar_1 = new UBar(FVector(0.0f, -0.95f, 0.0f), 1.0f, 0.15f, 0, EPlaySide::Up);

    //2번 플레이어가 움직이는 바
    Bar_2 = new UBar(FVector(0.0f, 0.95f, 0.0f), 1.0f, 0.15f, 1, EPlaySide::Down);

    UBall* newBall = UBall::CreateBallAtBar(*Bar_1);

    AddObject(newBall);
    AddObject(Bar_1);
    AddObject(Bar_2);

    //stage 블럭들
    CurrentStage = 2;
    stageblocks = CreateStage(CurrentStage);
    GetStageInfo(CurrentStage, CurrentStageRow, CurrentStageCol);

    //게임매니저 초기화
    gameManager = UGameManager::GetInstance();
    gameManager->RessetGM();
}

void UGameScene::Release()
{
    //Map에서 할당한 brick들을 해제해야함

    //생성된 모든 Ball을 제거
    for (UBall* ball : ActiveBallList)
    {
        if (ball != nullptr) {
            delete ball;
        }
    }
    ActiveBallList.clear();

    //생성된 모든 UGameObject을 제거
    for (UGameObject* Object : UGameObjectList)
    {
        if (Object != nullptr) {
            delete Object;
        }
    }

    UGameObjectList.clear();

    for (auto* b : stageblocks)
        delete b;

    stageblocks.clear();
}

/// <summary>
/// 해당 맵에 있는 모든 객체에 업데이트를 호출함
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

                    // 현재 충돌한 모서리 방향에 따른 인접 블록 조사 방향 설정
                    int dCol = (CollisionPos.x < b->CenterX) ? -1 : 1; // 왼쪽 모서리면 -1, 오른쪽이면 1
                    int dRow = (CollisionPos.y < b->CenterY) ? 1 : -1; // 아래쪽 모서리면 1, 위쪽이면 -1 (좌표계 확인 필요)

                    bool HasHorizontalBlock = false;
                    bool HasVerticalBlock = false;

                    // 1. 옆(수평) 방향에 블록이 있는지 확인
                    int CheckCol = CurCol + dCol;
                    if (CheckCol >= 0 && CheckCol < CurrentStageCol) {
                        if (stageblocks[CurRow * CurrentStageCol + CheckCol]
                            && stageblocks[CurRow * CurrentStageCol + CheckCol]->IsActive())
                            HasHorizontalBlock = true;
                    }

                    // 2. 위/아래(수직) 방향에 블록이 있는지 확인
                    int CheckRow = CurRow + dRow;
                    if (CheckRow >= 0 && CheckRow < CurrentStageRow) {
                        if (stageblocks[CheckRow * CurrentStageCol + CurCol]
                            && stageblocks[CheckRow * CurrentStageCol + CurCol]->IsActive())
                            HasVerticalBlock = true;
                    }

                    // 3. 판정 교정 로직
                    if (HasHorizontalBlock && !HasVerticalBlock) {
                        // 옆은 막혔고 위/아래가 비었음 -> 윗면/아랫면 충돌로 간주
                        CollisionState = EBlockCollision::Horizontal;
                        stageblocks[CurRow * CurrentStageCol + CheckCol]->SetSkipCalc(true);
                    }
                    else if (!HasHorizontalBlock && HasVerticalBlock) {
                        // 위/아래는 막혔고 옆이 비었음 -> 옆면 충돌로 간주
                        CollisionState = EBlockCollision::Vertical;
                        stageblocks[CheckRow * CurrentStageCol + CurCol]->SetSkipCalc(true);
                    }
                    // 둘 다 비어있으면(else) 원래의 Corner 판정 유지 (반사 벡터 사용)
                }

                (*ball).BallBounceAtBlock(CollisionState, *b, CollisionPos);
                gameManager->SetScore(b->GetScore()); 
                USoundManager::GetInstance().Play("Brick");
            }
            
        }
    }

    gameManager->Update(delta);

    // 밖에 공이 나갔는지 판별
    if (!HaveBalls())
    {
        // 공이 다 나갔으니 새 공을 하나 스폰해줍니다.
        UBall* newBall = UBall::CreateBallAtBar(*Bar_1);
        ActiveBallList.push_back(newBall);

        USoundManager::GetInstance().Play("Damage");
        gameManager->SubHealth(1);
    }

    if (bIsBrickEmpty()) // 벽돌 다 깨짐!
    {

        USoundManager::GetInstance().StopAll();
        USoundManager::GetInstance().Play("Victory");
        UClearScene::FinalScore = gameManager->GetTotalScore();
        USceneManager::GetInstance().LoadScene(ESceneType::Clear);
        return;
    }
}

/// <summary>
/// 모든 공을 확인하고 남은 게 있는지 여부를 반환
/// </summary>
/// <returns></returns>
bool UGameScene::HaveBalls()
{
    bool hasBallLeft = false;

    for (auto it = ActiveBallList.begin(); it != ActiveBallList.end(); )
    {
        UBall* ball = *it;

        //예외 처리
        if (ball == nullptr) {
            it = ActiveBallList.erase(it);
            continue;
        }

        FVector Location = ball->Location;
        float Radius = ball->Radius;

        //만약에 공이 밖으로 나가지 않았으면 다음거 확인
        if (Location.y < 1 - Radius && Location.y > -1 + Radius) {
            //공이 아직 남아있음
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
/// 공만 따로 구분짓는 코드 
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

    for (auto* b : stageblocks)
    {
        if (!b)         //<-added
            continue;
        b->Render(render);
    }
}

