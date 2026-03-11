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


//// 공 생성 함수
//static UBall* CreateBall()
//{
//    // new 연산자를 사용해 공의 Instance를 생성
//    UBall* Ball = new UBall();
//
//    // 임의의 크기(Radius): 너무 큰 값을 방지하기 위해, 공의 크기를 화면 너비의 1/10로 제한
//    float maxRadiusX = (rightBorder - leftBorder) * 0.05f;
//    float maxRadiusY = (topBorder - bottomBorder) * 0.05f;
//    float maxAllowedRadius = (maxRadiusX < maxRadiusY) ? maxRadiusX : maxRadiusY;
//    float r = maxAllowedRadius / 2;
//    Ball->SetRadius(r);
//
//    // 임의의 위치(Location): 화면 경계 안쪽의 랜덤한 위치, 반지름을 마진값으로 함
//    Ball->Velocity.x = GetRandomFloat(0.5f, 0.6f);
//    Ball->Velocity.y = GetRandomFloat(0.5f, 0.6f);
//    Ball->Location.z = 0.0f;
//
//    // 임의의 속도(Velocity)
//    Ball->Velocity.x = 1.0f;
//    Ball->Velocity.y = -1.0f;
//    Ball->Velocity.z = 0.0f;
//    Ball->Speed = 1.f;
//
//    return Ball;
//}

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
    int CurrentRound = 1;
    stageblocks = CreateStage(CurrentRound);

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
    

        for (auto* b : stageblocks)
        {
            if (!b->IsActive()) continue;

            EBlockCollision CollisionState = (*ball).CheckBlockCollision(*b, CollisionPos);
            if (CollisionState != EBlockCollision::None)
            {
                (*ball).BallBounceAtBlock(CollisionState, *b, CollisionPos);
                gameManager->SetScore(b->GetScore()); 
                USoundManager::GetInstance().Play("Brick");
            }
        }
    }

    // Item Objects Update
    UItemManager::Get().Update(delta);
    UItemManager::Get().CheckCollision(Bar_1);
    UItemManager::Get().CheckCollision(Bar_2);

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
        // 아이템 관련 리소스 해제
        UItemManager::Get().Clear();

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
        if (b->IsActive()) return false;
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
        b->Render(render);
}

