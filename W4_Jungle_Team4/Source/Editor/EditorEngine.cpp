#include "Editor/EditorEngine.h"

#include "Editor/Slate/SlateApplication.h"
#include "Editor/Slate/SViewport.h"
#include "Editor/Slate/SSplitterH.h"
#include "Editor/Slate/SSplitterV.h"
#include "Editor/Slate/SImGuiConsoleWidget.h"
#include "Editor/Slate/SImGuiControlWidget.h"
#include "Editor/Slate/SImGuiSceneWidget.h"
#include "Editor/Slate/SImGuiPropertyWidget.h"
#include "Editor/Viewport/Viewport.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Engine/Serialization/SceneSaveManager.h"
#include "Component/GizmoComponent.h"
#include "Component/CameraComponent.h"
#include "Component/PrimitiveComponent.h"
#include "GameFramework/World.h"
#include "Editor/EditorRenderPipeline.h"
#include "Editor/Viewport/ViewportManager.h"
#include "Core/Stats.h"

DEFINE_CLASS(UEditorEngine, UEngine)
REGISTER_FACTORY(UEditorEngine)

void UEditorEngine::Init(FWindowsWindow* InWindow)
{
	// 엔진 공통 초기화 (Renderer, D3D, 싱글턴 등)
	UEngine::Init(InWindow);

	// World
	if (WorldList.empty())
	{
		CreateWorldContext(EWorldType::Editor, FName("Default"));
	}
	SetActiveWorld(WorldList[0].ContextHandle);
	GetWorld()->InitWorld();

	// Selection & Gizmo
	SelectionManager.Init();

	// Viewport Manager
	FViewportManager::Get().Initialize(&Renderer.GetFD3DDevice(), GetWorld(), &SelectionManager);

	// 에디터 전용 초기화
	FEditorSettings::Get().LoadFromFile(FEditorSettings::GetDefaultSettingsPath());

	MainPanel.Create(Window, Renderer, this);

	// TODO: Default Slate UI 생성 메서드 분리 필요
	if (FSlateApplication::Get().GetWindows().empty())
	{
		FViewport* Viewport = FViewportManager::Get().CreateViewport();
		FEditorViewportClient* ViewportClient = FViewportManager::Get().CreateEditorViewportClient();
		ViewportClient->Initialize(Viewport);
		Viewport->SetViewportClient(ViewportClient);

		SWindow* MainWindow = new SWindow();
		SSplitterH* SplitterH = new SSplitterH();
		SSplitterH* SplitterH2 = new SSplitterH();
		SSplitterV* SplitterV = new SSplitterV();
		SSplitterV* SplitterV2 = new SSplitterV();
		SSplitterV* SplitterV3 = new SSplitterV();

		SViewport* ViewportWidget = new SViewport();
		ViewportWidget->SetViewport(Viewport);

		SImGuiConsoleWidget* ConsoleWidget = new SImGuiConsoleWidget();
		SImGuiControlWidget* ControlWidget = new SImGuiControlWidget(this);
		SImGuiSceneWidget* SceneWidget = new SImGuiSceneWidget(this);
		SImGuiPropertyWidget* PropertyWidget = new SImGuiPropertyWidget(this);

		FRect ViewportRect = { 0, 0, InWindow->GetWidth(), InWindow->GetHeight() };
		MainWindow->SetRect(ViewportRect);
		SplitterV->SetRect(ViewportRect);

		MainWindow->AddChild(SplitterV);
		SplitterV->SetSideLT(SplitterH);
		SplitterV->SetSideRB(SplitterH2);

		SplitterH->SetSideLT(SplitterV2);
		SplitterH->SetSideRB(ConsoleWidget);

		SplitterV2->SetSideLT(ControlWidget);
		SplitterV2->SetSideRB(ViewportWidget);

		SplitterH2->SetSideLT(SceneWidget);
		SplitterH2->SetSideRB(PropertyWidget);

		FSlateApplication::Get().AddWindow(MainWindow);
	}

	// Main Camera는 어떤걸로?
	if (!FViewportManager::Get().GetViewports().empty())
	{
		FViewportClient* EditorViewportClient = FViewportManager::Get().GetViewports()[0]->GetViewportClient();

		if (FEditorViewportClient* EVC = dynamic_cast<FEditorViewportClient*>(EditorViewportClient))
		{
			GetWorld()->SetActiveCamera(EVC->GetCamera());
		}
	}

	// Editor render pipeline
	SetRenderPipeline(std::make_unique<FEditorRenderPipeline>(this, Renderer));
}

void UEditorEngine::Shutdown()
{
	// 에디터 해제 (엔진보다 먼저)
	FEditorSettings::Get().SaveToFile(FEditorSettings::GetDefaultSettingsPath());
	CloseScene();
	SelectionManager.Shutdown();
	MainPanel.Release();

	// 엔진 공통 해제 (Renderer, D3D 등)
	UEngine::Shutdown();
}

void UEditorEngine::OnWindowResized(uint32 Width, uint32 Height)
{
	UEngine::OnWindowResized(Width, Height);
}

void UEditorEngine::Tick(float DeltaTime)
{
	FSlateApplication::Get().Tick(DeltaTime);
	MainPanel.Update();
	UEngine::Tick(DeltaTime);
}

void UEditorEngine::RenderUI(float DeltaTime)
{
	MainPanel.BeginFrame();
	MainPanel.Render(DeltaTime);
}

void UEditorEngine::ResetViewport()
{
	//ViewportClient.CreateCamera();
	//ViewportClient.SetWorld(GetWorld());
	//ViewportClient.SetViewportSize(Window->GetWidth(), Window->GetHeight());
	//ViewportClient.ResetCamera();
	GetWorld()->SetActiveCamera(GetWorld()->GetActiveCamera());
}

void UEditorEngine::CloseScene()
{
	SelectionManager.ClearSelection();

	for (FWorldContext& Ctx : WorldList) {
		Ctx.World->EndPlay();
		UObjectManager::Get().DestroyObject(Ctx.World);
	}
	WorldList.clear();
	ActiveWorldHandle = FName::None;

	//ViewportClient.DestroyCamera();
	//ViewportClient.SetWorld(nullptr);
}

void UEditorEngine::NewScene()
{
	ClearScene();
	FWorldContext& Ctx = CreateWorldContext(EWorldType::Editor, FName("NewScene"), "New Scene");
	SetActiveWorld(Ctx.ContextHandle);

	ResetViewport();
}

void UEditorEngine::ClearScene()
{
	SelectionManager.ClearSelection();

	for (FWorldContext& Ctx : WorldList)
	{
		Ctx.World->EndPlay();
		UObjectManager::Get().DestroyObject(Ctx.World);
	}

	WorldList.clear();
	ActiveWorldHandle = FName::None;

	//ViewportClient.DestroyCamera();
	//ViewportClient.SetWorld(nullptr);
}
