#include "EditorRenderPipeline.h"

#include "Editor/EditorEngine.h"
#include "Editor/Viewport/Viewport.h"
#include "Editor/Viewport/ViewportManager.h"
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

	for (FViewport* Viewport : FViewportManager::Get().GetViewports())
	{
		Bus.Clear();

		UWorld* World = Editor->GetWorld();
		FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(Viewport->GetViewportClient());
		UCameraComponent* Camera = ViewportClient ? ViewportClient->GetCamera() : nullptr;

		if (Camera)
		{
			const auto& Settings = Editor->GetSettings();
			const FShowFlags& ShowFlags = ViewportClient->GetShowFlags();
			EViewMode ViewMode = ViewportClient->GetViewMode();

			Bus.SetViewProjection(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(),
				Camera->GetRightVector(), Camera->GetUpVector());
			Bus.SetRenderSettings(ViewMode, ShowFlags);

			// Gizmo는 렌더링 직전에 뷰포트 내 카메라를 이용해서 스케일링을 적용합니다.
			// 이는 Gizmo를 picking할 경우에도 예외적으로 처리합니다.
			Editor->GetGizmo()->ApplyScreenSpaceScaling(Camera->GetWorldLocation());

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

	for (FViewport* Viewport : FViewportManager::Get().GetViewports())
	{
		Renderer.GetViewportRenderer().Render(Renderer.GetFD3DDevice().GetDeviceContext(), Viewport);
	}

	Editor->RenderUI(DeltaTime);
	Renderer.EndFrame();
}
