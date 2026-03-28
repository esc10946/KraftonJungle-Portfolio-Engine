#include "Editor/EditorEngine.h"

#include "Editor/Slate/SlateApplication.h"
#include "Editor/Slate/SlateWidgetFactory.h"
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
	FSlateWidgetFactory::LoadViewportLayouts();

	// 에디터 전용 초기화
	FEditorSettings::Get().LoadFromFile(FEditorSettings::GetDefaultSettingsPath());

	MainPanel.Create(Window, Renderer, this);

	// Default Slate UI 생성
	if (FSlateApplication::Get().GetWindows().empty())
	{
		SWindow* MainWindow = new SWindow();
		FRect ViewportRect = { 0, 0, InWindow->GetWidth(), InWindow->GetHeight() };
		MainWindow->SetRect(ViewportRect);

		SWidget* RootWidget = FSlateWidgetFactory::LoadRecursive("Default");
		if (RootWidget)
		{
			RootWidget->SetRect(ViewportRect);
			MainWindow->AddChild(RootWidget);
		}

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

UCameraComponent* UEditorEngine::GetCamera() const
{
	for (FViewport* Viewport : FViewportManager::Get().GetViewports())
	{
		if (!Viewport)
		{
			continue;
		}

		if (FEditorViewportClient* ViewportClient = dynamic_cast<FEditorViewportClient*>(Viewport->GetViewportClient()))
		{
			return ViewportClient->GetCamera();
		}
	}

	return nullptr;
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
