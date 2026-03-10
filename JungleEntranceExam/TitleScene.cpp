#include "TitleScene.h"
#include "USceneManager.h"
#include "URenderer.h"

void UTitleScene::Init()
{
    UGameObjectList.clear();
    SceneType = ESceneType::Title;

    //UI설정 
    // 1. 타이틀 로고 생성 및 배치
    // TitleLogo = new UGameObject();
    // TitleLogo->SetPosition(0, 50);
    // AddObject(TitleLogo);

    // 2. 시작 버튼 생성 및 배치
    // StartButton = new UUIButton();
    // StartButton->SetPosition(0, -20);
    // AddObject(StartButton);

    // 3. 종료 버튼 생성 및 배치
    // EndButton = new UUIButton();
    // EndButton->SetPosition(0, -50);
    // AddObject(EndButton);
    // 
    // 4. 크레딧 버튼 생성 및 배치
    // CreditButton = new UUIButton();
    // EndButton->SetPosition(0, -50);
    // AddObject(CreditButton);
}

void UTitleScene::Update(float delta)
{
    // for (UGameObject* obj : UGameObjectList) { obj->Update(delta); }

    // 마우스 왼쪽 버튼이 눌렸는지 확인 (예시 API)
    // if (InputManager::GetInstance()->GetKeyDown(VK_LBUTTON)) 
    // {
    //     // 시작 버튼 영역 안에 마우스가 있다면?
    //     if (StartButton->IsMouseHovered()) {
    //         GameStart();
    //     }
    //     // 종료 버튼 영역 안에 마우스가 있다면?
    //     else if (EndButton->IsMouseHovered()) {
    //         GameEnd();
    //     }
    // }
}

void UTitleScene::Render(URenderer render)
{
}

void UTitleScene::Release()
{
    // UScene에 만들어둔 기본 Release 로직 (UGameObjectList 비우기) 호출
}

void UTitleScene::GameStart()
{
    USceneManager::GetInstance().LoadScene(ESceneType::InGame);
}

void UTitleScene::GameEnd()
{
    PostQuitMessage(0);
}

void UTitleScene::Credit()
{

}

