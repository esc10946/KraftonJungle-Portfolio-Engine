#include "UGameScene.h"
#include "UGameObject.h"
#include "UGameManager.h" 
#include "UBall.h"

UGameScene::UGameScene()
{
    Init(); 
}

UGameScene::~UGameScene()
{
    Release(); 
}

//해당 게임에서 생성되는 모든 오브젝트여기서 생성
void UGameScene::Init()
{
    UGameObjectList.clear();
    SceneType = ESceneType::InGame;

    ActiveBallList.clear();
    //볼의 위치 혹은 속도 초기화 --
    ActiveBallList.push_back(new UBall());

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
        if (Object != nullptr) {
            //Object->Update(delta);
        }
    }

    gameManager->Update(delta);

    // 밖에 공이 나갔는지 판별
    if (!HaveBalls())
    {
        // 게임 매니저에게 체력한칸을 줄여달라고함
        gameManager->SubHealth(1);

        //공의 속도, 위치 초기화
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
