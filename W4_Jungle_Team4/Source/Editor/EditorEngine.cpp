#include "Editor/EditorEngine.h"

#include "Editor/Slate/SlateApplication.h"
#include "Editor/Slate/SViewport.h"
#include "Editor/Slate/SSplitterH.h"
#include "Editor/Viewport/Viewport.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Engine/Serialization/SceneSaveManager.h"
#include "Component/GizmoComponent.h"
#include "Component/CameraComponent.h"
#include "Component/PrimitiveComponent.h"
#include "GameFramework/World.h"
#include "Editor/EditorRenderPipeline.h"
#include "Core/Stats.h"

DEFINE_CLASS(UEditorEngine, UEngine)
REGISTER_FACTORY(UEditorEngine)

void UEditorEngine::Init(FWindowsWindow* InWindow)
{
	// 엔진 공통 초기화 (Renderer, D3D, 싱글턴 등)
	UEngine::Init(InWindow);

	// 에디터 전용 초기화
	FEditorSettings::Get().LoadFromFile(FEditorSettings::GetDefaultSettingsPath());

	MainPanel.Create(Window, Renderer, this);

	// World
	if (WorldList.empty())
	{
		CreateWorldContext(EWorldType::Editor, FName("Default"));
	}
	SetActiveWorld(WorldList[0].ContextHandle);
	GetWorld()->InitWorld();

	// Selection & Gizmo
	SelectionManager.Init();

	// Slate UI 초기화
	FViewport* Viewport = new FViewport(&Renderer.GetFD3DDevice());
	FViewport* Viewport2 = new FViewport(&Renderer.GetFD3DDevice());

	FEditorViewportClient* EditorViewportClient = new FEditorViewportClient();
	EditorViewportClient->SetSettings(&FEditorSettings::Get());
	EditorViewportClient->Initialize(Viewport);
	EditorViewportClient->SetWorld(GetWorld());
	EditorViewportClient->SetGizmo(SelectionManager.GetGizmo());
	EditorViewportClient->SetSelectionManager(&SelectionManager);
	EditorViewportClient->CreateCamera();
	EditorViewportClient->ResetCamera();

	Viewport->SetViewportClient(EditorViewportClient);
	Viewport2->SetViewportClient(EditorViewportClient);
	Viewports.push_back(Viewport);
	Viewports.push_back(Viewport2);

	SWindow* MainWindow = new SWindow();
	SSplitterH* SplitterH = new SSplitterH();

	SViewport* ViewportWidget = new SViewport();
	ViewportWidget->SetViewport(Viewport);
	SViewport* ViewportWidget2 = new SViewport();
	ViewportWidget2->SetViewport(Viewport2);

	FRect ViewportRect = { 0, 0, 800, 600 };
	MainWindow->SetRect(ViewportRect);
	SplitterH->SetRect(ViewportRect);

	ViewportWidget->SetViewport(Viewport);
	MainWindow->AddChild(SplitterH);

	SplitterH->SetSideLT(ViewportWidget);
	SplitterH->SetSideRB(ViewportWidget2);

	FSlateApplication::Get().AddWindow(MainWindow);

	GetWorld()->SetActiveCamera(EditorViewportClient->GetCamera());

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
	
	for (FViewport* Viewport : Viewports)
	{
		FViewportClient* Client = Viewport->GetViewportClient();
		if (Client)
		{
			Client->OnViewportResized(Width, Height);
		}
	}
}

void UEditorEngine::Tick(float DeltaTime)
{
	FSlateApplication::Get().Tick(DeltaTime);
	MainPanel.Update();
	UEngine::Tick(DeltaTime);
}

void UEditorEngine::RenderUI(float DeltaTime)
{
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
