#include "EditorRenderPipeline.h"

#include "Editor/EditorEngine.h"
#include "Editor/Viewport/Viewport.h"
#include "Render/Renderer/Renderer.h"
#include "Component/CameraComponent.h"
#include "Component/GizmoComponent.h"
#include "GameFramework/World.h"
#include "Core/Stats.h"
#include "Core/GPUProfiler.h"

FEditorRenderPipeline::FEditorRenderPipeline(UEditorEngine* InEditor, FRenderer& InRenderer)
	: Editor(InEditor)
{
	Collector.Initialize(InRenderer.GetFD3DDevice().GetDevice());
}

FEditorRenderPipeline::~FEditorRenderPipeline()
{
	Collector.Release();
}

void FEditorRenderPipeline::Execute(float DeltaTime, FRenderer& Renderer)
{
#if STATS
	FStatManager::Get().TakeSnapshot();
	FGPUProfiler::Get().TakeSnapshot();
#endif

	for (FViewport* Viewport : Editor->GetViewports())
	{
		Bus.Clear();

		UWorld* World = Editor->GetWorld();
		FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(Viewport->GetViewportClient());
		UCameraComponent* Camera = ViewportClient ? ViewportClient->GetCamera() : nullptr;

		if (Camera)
		{
			const auto& Settings = Editor->GetSettings();
			const FShowFlags& ShowFlags = Settings.ShowFlags;
			EViewMode ViewMode = Settings.ViewMode;

			Bus.SetViewProjection(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(),
				Camera->GetRightVector(), Camera->GetUpVector());
			Bus.SetRenderSettings(ViewMode, ShowFlags);

			Collector.CollectWorld(World, ShowFlags, ViewMode, Bus);
			Collector.CollectGrid(Settings.GridSpacing, Settings.GridHalfLineCount, Bus);
			Collector.CollectGizmo(Editor->GetGizmo(), ShowFlags, Bus);
			Collector.CollectSelection(
				Editor->GetSelectionManager().GetSelectedActors(),
				ShowFlags, ViewMode, Bus);

			Renderer.PrepareBatchers(Bus);
			Renderer.BeginFrame(Viewport->GetRenderTarget());
			Renderer.Render(Bus);
			Renderer.EndViewportFrame(Viewport->GetRenderTarget());
		}
	}

	Bus.Clear();

	Renderer.BeginFrame();

	for (FViewport* Viewport : Editor->GetViewports())
	{
		Renderer.ViewportRenderer.Render(Renderer.GetFD3DDevice().GetDeviceContext(), Viewport);
	}

	Editor->RenderUI(DeltaTime);
	Renderer.EndFrame();
}
