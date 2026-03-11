#include "TitleScene.h"
#include "USceneManager.h"
#include "UsoundManager.h"
#include "URenderer.h"
#include "UGameObject.h"
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
    //?앹꽦??紐⑤뱺 UGameObject???쒓굅
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
    USoundManager::GetInstance().Play("Start");
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

    // 1. 李쎌쓣 ?붾㈃ ?뺤쨷?숈뿉 諛곗튂
    ImVec2 centerPos = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
        viewport->WorkPos.y + viewport->WorkSize.y * 0.5f
    );
    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGuiWindowFlags hudFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Title", nullptr, hudFlags);

    float windowWidth = ImGui::GetWindowWidth();

    const char* titleText = "Brick Breaker";
    ImGui::SetWindowFontScale(8.0f);

    // 湲?먭? 李⑥???媛濡?湲몄씠瑜?怨꾩궛?섍퀬 而ㅼ꽌瑜??대룞?쒗궢?덈떎.
    float titleWidth = ImGui::CalcTextSize(titleText).x;
    ImGui::SetCursorPosX((windowWidth - titleWidth) * 0.5f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), titleText);

    ImGui::Text("\n"); // 以꾨컮轅?

    // ==========================================
    // 2. 踰꾪듉 以묒븰 ?뺣젹 
    // ==========================================
    ImGui::SetWindowFontScale(4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.6f, 1.0f, 1.0f, 0.1f));

    // ?듭씪媛먯쓣 ?꾪빐 踰꾪듉 ?ш린瑜?怨좎젙?⑸땲??
    float btnWidth = 400.0f;
    float btnHeight = 80.0f;
    ImVec2 btnSize(btnWidth, btnHeight);

    // 寃뚯엫 ?쒖옉 踰꾪듉
    ImGui::SetCursorPosX((windowWidth - btnWidth) * 0.5f);
    if (ImGui::Button("Game Start", btnSize))
    {
        USceneManager::GetInstance().LoadScene(ESceneType::InGame);
    }

    ImGui::Text("\n");

    // 寃뚯엫 醫낅즺 踰꾪듉
    ImGui::SetCursorPosX((windowWidth - btnWidth) * 0.5f);
    if (ImGui::Button("Quit", btnSize))
    {
        GameEnd();
    }

    ImGui::Text("\n");
    ImGui::PopStyleColor(3); // 踰꾪듉 ?ㅽ????먯긽蹂듦뎄

    ImGui::SetWindowFontScale(1.5f);
    const char* teamText = "Team 8: Kim Hyung-do,  Kim Ho-jun,  Jo Sang-hyeon,  Kim Sun-myeong";

    float teamWidth = ImGui::CalcTextSize(teamText).x;
    ImGui::SetCursorPosX((windowWidth - teamWidth) * 0.5f);
    ImGui::Text(teamText);

    // ==========================================

    ImGui::End();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
