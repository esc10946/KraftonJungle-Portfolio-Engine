#include "TitleScene.h"
#include "USceneManager.h"

void UTitleScene::GameStart()
{
    USceneManager::GetInstance().LoadScene(ESceneType::InGame);
}

void UTitleScene::GameEnd()
{

}

void UTitleScene::Credit()
{

}

void UTitleScene::Init()
{
    UGameObjectList.clear();
    SceneType = ESceneType::Title;

    //UI설정 
    // 게임 제목(Brick Breaker)
    // 게임 시작
    // 게임 종료
    // 크레딧
}

void UTitleScene::Release()
{
    //UI전부 제외
}
