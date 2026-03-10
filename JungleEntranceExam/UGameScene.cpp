#include "UGameScene.h"
#include "UGameObject.h"
#include "UGameManager.h" 
#include "UBall.h"
#include "UBar.h"
#include "Util.h"

UGameScene::UGameScene()
{
    Init(); 
}

UGameScene::~UGameScene()
{
    Release(); 
}


// 공 생성 함수
static UBall* CreateBall()
{
    // new 연산자를 사용해 공의 Instance를 생성
    UBall* Ball = new UBall();

    // 임의의 크기(Radius): 너무 큰 값을 방지하기 위해, 공의 크기를 화면 너비의 1/10로 제한
    float maxRadiusX = (rightBorder - leftBorder) * 0.05f;
    float maxRadiusY = (topBorder - bottomBorder) * 0.05f;
    float maxAllowedRadius = (maxRadiusX < maxRadiusY) ? maxRadiusX : maxRadiusY;
    float r = GetRandomFloat(0.01f, maxAllowedRadius);
    Ball->SetRadius(r);

    // 임의의 위치(Location): 화면 경계 안쪽의 랜덤한 위치, 반지름을 마진값으로 함
    Ball->Location.x = GetRandomFloat(leftBorder + Ball->Radius, rightBorder - Ball->Radius);
    Ball->Location.y = GetRandomFloat(bottomBorder + Ball->Radius, topBorder - Ball->Radius);
    Ball->Location.z = 0.0f;

    // 임의의 속도(Velocity)
    Ball->Velocity.x = 1.0f;
    Ball->Velocity.y = 1.0f;
    Ball->Velocity.z = 0.0f;

    return Ball;
}

//해당 게임에서 생성되는 모든 오브젝트여기서 생성
void UGameScene::Init()
{
    UGameObjectList.clear();
    SceneType = ESceneType::InGame;

    ActiveBallList.clear();

    UBall* newBall = CreateBall();

    //1번 플레이어가 움직이는 바
    UBar* Bar_1 = new UBar(FVector(0.0f, -0.95f, 0.0f), 0.7f, 0.1f, 0);

    //2번 플레이어가 움직이는 바
    UBar* Bar_2 = new UBar(FVector(0.0f, 0.95f, 0.0f), 0.7f, 0.1f, 0);

    AddObject(newBall);
    AddObject(Bar_1);
    AddObject(Bar_2);

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
}

/// <summary>
/// 해당 맵에 있는 모든 객체에 업데이트를 호출함
/// </summary>
/// <param name="delta"></param>
void UGameScene::Update(float delta)
{
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
    }

    gameManager->Update(delta);

    // 밖에 공이 나갔는지 판별
    if (!HaveBalls())
    {
        gameManager->SubHealth(1);

        // 공이 다 나갔으니 새 공을 하나 스폰해줍니다.
        UBall* newBall = CreateBall();
        ActiveBallList.push_back(newBall);
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
        if (Location.y < 1 + Radius && Location.y > -1 - Radius) {
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
}

