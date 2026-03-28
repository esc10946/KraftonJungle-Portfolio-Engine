#include "Editor/EditorEngine.h"

#include "Editor/Slate/SlateApplication.h"
#include "Editor/Slate/SViewport.h"
#include "Editor/Slate/SSplitterH.h"
#include "Editor/Slate/SSplitterV.h"
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
	FViewport* Viewport3 = new FViewport(&Renderer.GetFD3DDevice());
	FViewport* Viewport4 = new FViewport(&Renderer.GetFD3DDevice());

	FEditorViewportClient* EditorViewportClient = new FEditorViewportClient();
	EditorViewportClient->SetSettings(&FEditorSettings::Get());
	EditorViewportClient->Initialize(Viewport);
	EditorViewportClient->SetWorld(GetWorld());
	EditorViewportClient->SetGizmo(SelectionManager.GetGizmo());
	EditorViewportClient->SetSelectionManager(&SelectionManager);
	EditorViewportClient->CreateCamera();
	EditorViewportClient->ResetCamera();

	FEditorViewportClient* EditorViewportClient2 = new FEditorViewportClient();
	EditorViewportClient2->SetSettings(&FEditorSettings::Get());
	EditorViewportClient2->Initialize(Viewport2);
	EditorViewportClient2->SetWorld(GetWorld());
	EditorViewportClient2->SetGizmo(SelectionManager.GetGizmo());
	EditorViewportClient2->SetSelectionManager(&SelectionManager);
	EditorViewportClient2->CreateCamera();
	EditorViewportClient2->ResetCamera();

	FEditorViewportClient* EditorViewportClient3 = new FEditorViewportClient();
	EditorViewportClient3->SetSettings(&FEditorSettings::Get());
	EditorViewportClient3->Initialize(Viewport3);
	EditorViewportClient3->SetWorld(GetWorld());
	EditorViewportClient3->SetGizmo(SelectionManager.GetGizmo());
	EditorViewportClient3->SetSelectionManager(&SelectionManager);
	EditorViewportClient3->CreateCamera();
	EditorViewportClient3->ResetCamera();

	FEditorViewportClient* EditorViewportClient4 = new FEditorViewportClient();
	EditorViewportClient4->SetSettings(&FEditorSettings::Get());
	EditorViewportClient4->Initialize(Viewport4);
	EditorViewportClient4->SetWorld(GetWorld());
	EditorViewportClient4->SetGizmo(SelectionManager.GetGizmo());
	EditorViewportClient4->SetSelectionManager(&SelectionManager);
	EditorViewportClient4->CreateCamera();
	EditorViewportClient4->ResetCamera();

	Viewport->SetViewportClient(EditorViewportClient);
	Viewport2->SetViewportClient(EditorViewportClient2);
	Viewport3->SetViewportClient(EditorViewportClient3);
	Viewport4->SetViewportClient(EditorViewportClient4);
	Viewports.push_back(Viewport);
	Viewports.push_back(Viewport2);
	Viewports.push_back(Viewport3);
	Viewports.push_back(Viewport4);

	SWindow* MainWindow = new SWindow();
	SSplitterH* SplitterH = new SSplitterH();
	SSplitterV* SplitterV = new SSplitterV();
	SSplitterV* SplitterV2 = new SSplitterV();

	SViewport* ViewportWidget = new SViewport();
	ViewportWidget->SetViewport(Viewport);
	SViewport* ViewportWidget2 = new SViewport();
	ViewportWidget2->SetViewport(Viewport2);
	SViewport* ViewportWidget3 = new SViewport();
	ViewportWidget3->SetViewport(Viewport3);
	SViewport* ViewportWidget4 = new SViewport();
	ViewportWidget4->SetViewport(Viewport4);

	FRect ViewportRect = { 100, 100, 1500, 800 };
	MainWindow->SetRect(ViewportRect);
	SplitterH->SetRect(ViewportRect);

	MainWindow->AddChild(SplitterH);
	SplitterH->SetSideLT(SplitterV);
	SplitterH->SetSideRB(SplitterV2);

	SplitterV->SetSideLT(ViewportWidget);
	SplitterV->SetSideRB(ViewportWidget2);
	SplitterV2->SetSideLT(ViewportWidget3);
	SplitterV2->SetSideRB(ViewportWidget4);

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
