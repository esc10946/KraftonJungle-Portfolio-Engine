#include "Editor/EditorEngine.h"

#include "Editor/Slate/SlateApplication.h"
#include "Editor/Slate/SlateWidgetFactory.h"
#include "Editor/Slate/Widget/SViewport.h"
#include "Editor/Slate/ConsoleCommandManager.h"
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

#include "AssetPipeline/Obj/AssetImportManager.h"
#include "Asset/StaticMesh/StaticMeshManager.h"
#include "GameFramework/PrimitiveActors.h"
#include "Component/StaticMeshComponent.h"

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

	// 1회 resizing
	OnWindowResized(InWindow->GetWidth(), InWindow->GetHeight());

	// Console Command 등록 (어디서 해야할지 애매해서 일단 여기)
	FConsoleCommandManager::Get().RegisterCommand("stat", [this](const TArray<FString>& Args) {
		if (Args.size() < 2)
		{
			SImGuiConsoleWidget::AddLog("Usage:\n");
			SImGuiConsoleWidget::AddLog("  stat fps     - Show FPS stats on the focused viewport.\n");
			SImGuiConsoleWidget::AddLog("  stat memory  - Show memory stats on the focused viewport.\n");
			SImGuiConsoleWidget::AddLog("  stat none    - Hide all stats on the focused viewport.\n");
			return;
		}

		SViewport* FocusedViewport = FViewportManager::Get().GetFocusedViewportWidget();
		if (FocusedViewport == nullptr)
		{
			SImGuiConsoleWidget::AddLog("[ERROR] No focused viewport to apply stat command.\n");
			return;
		}

		if (Args[1] == "fps")
		{
			FocusedViewport->SetShowFPS(true);
		}
		else if (Args[1] == "memory")
		{
			FocusedViewport->SetShowMemory(true);
		}
		else if (Args[1] == "none")
		{
			FocusedViewport->SetShowFPS(false);
			FocusedViewport->SetShowMemory(false);
		}
		else
		{
			SImGuiConsoleWidget::AddLog("Usage:\n");
			SImGuiConsoleWidget::AddLog("  stat fps     - Show FPS stats on the focused viewport.\n");
			SImGuiConsoleWidget::AddLog("  stat memory  - Show memory stats on the focused viewport.\n");
			SImGuiConsoleWidget::AddLog("  stat none    - Hide all stats on the focused viewport.\n");
		}
		});
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
	FSlateApplication::Get().OnWindowResized(Width, Height);
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
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FViewport*>& Viewports = FViewportManager::Get().GetViewports();
	FEditorViewportClient* FirstViewportClient = nullptr;

	for (FViewport* Viewport : Viewports)
	{
		if (!Viewport)
		{
			continue;
		}
		if (FEditorViewportClient* ViewportClient = dynamic_cast<FEditorViewportClient*>(Viewport->GetViewportClient()))
		{
			ViewportClient->SetWorld(World);

			if (FirstViewportClient == nullptr)
			{
				FirstViewportClient = ViewportClient;
			}
		}
	}

	if (FirstViewportClient && FirstViewportClient->GetCamera())
	{
		World->SetActiveCamera(FirstViewportClient->GetCamera());
	}
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
	Ctx.World->InitWorld();
	FViewportManager::Get().SetWorld(Ctx.World);

	ResetViewport();
}

void UEditorEngine::ClearScene()
{
	SelectionManager.ClearSelection();
	FViewportManager::Get().SetWorld(nullptr);

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

bool UEditorEngine::HandleObjImported(const FString& SourceObjPath)
{
	FImportedObjPackage ImportedPackage;
	if (!FAssetImportManager::ImportObjPackage(SourceObjPath, ImportedPackage))
	{
		return false;
	}

	UStaticMesh* MeshObj =
		FStaticMeshManager::Get().LoadStaticMeshObject(ImportedPackage.ImportedObjPath);

	if (!MeshObj)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>();
	if (!Actor)
	{
		return false;
	}

	Actor->InitDefaultComponents();
	Actor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f)); // 필요하면 나중에 드롭 위치 기준으로 바꿔도 됨

	if (UStaticMeshComponent* MeshComp = UStaticMeshComponent::Cast(Actor->GetRootComponent()))
	{
		MeshComp->SetStaticMesh(MeshObj);
		return true;
	}

	return false;
}