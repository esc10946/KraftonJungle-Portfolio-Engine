#include "TitleScene.h"
#include "USceneManager.h"
#include "URenderer.h"

void UTitleScene::Init()
{
    UGameObjectList.clear();
    SceneType = ESceneType::Title;

}

void UTitleScene::Update(float delta)
{
    
}

void UTitleScene::Render(URenderer render)
{
}

void UTitleScene::Release()
{
    //생성된 모든 UGameObject을 제거
    for (UGameObject* Object : UGameObjectList)
    {
        if (Object != nullptr) {
            delete Object;
        }
    }

    UGameObjectList.clear();
}

void UTitleScene::GameStart()
{
    USceneManager::GetInstance().LoadScene(ESceneType::InGame);
}

void UTitleScene::GameEnd()
{
    PostQuitMessage(0);
}

void UTitleScene::UIRender()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float padding = 200.f;
    ImVec2 centerPos = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
        viewport->WorkPos.y + viewport->WorkSize.y * 0.5f
    );

    // 2. 좌표는 화면 중앙(centerPos), 피봇도 창의 중앙(0.5f, 0.5f)으로 세팅!
    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGuiWindowFlags hudFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Title", nullptr, hudFlags);


    ImGui::SetWindowFontScale(5.0f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Brick Braker");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "\n");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "\n");

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.0f));

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.0f));

    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 0.0f));


    //게임 시작 버튼
    if (ImGui::Button("Game Start"))
    {
        USceneManager::GetInstance().LoadScene(ESceneType::InGame);
    }
    ImGui::Text("\n");

    //게임 종료 버튼
    if (ImGui::Button("Quit"))
    {
        GameEnd();
    }
    ImGui::Text("\n");

    ImGui::PopStyleColor(3);
    ImGui::SetWindowFontScale(2.0f);

    ImGui::Text("Team 8: Kim Hyung-do,  Kim Ho-jun,  Jo Sang-hyeon,  Kim Sun-myeong");
    ImGui::End();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

