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

	for (int i = 0; i < 1; ++i)
	{
		FViewport* Viewport = FViewportManager::Get().GetViewports()[i];

		Bus.Clear();

		UWorld* World = Editor->GetWorld();
		FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(Viewport->GetViewportClient());
		UCameraComponent* Camera = ViewportClient ? ViewportClient->GetCamera() : nullptr;

		if (Camera)
		{
			const auto& Settings = Editor->GetSettings();

			FViewOption ViewOption = ViewportClient->GetCurrentViewOption();
			const FShowFlags& ShowFlags = ViewOption.ShowFlags;
			EViewMode ViewMode = ViewOption.ViewMode;

			Bus.SetViewProjection(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(),
				Camera->GetRightVector(), Camera->GetUpVector());
			Bus.SetOrthographic(Camera->IsOrthogonal());
			Bus.SetRenderSettings(ViewMode, ShowFlags);

			// Gizmo는 렌더링 직전에 뷰포트 내 카메라를 이용해서 스케일링을 적용합니다.
			// 이는 Gizmo를 picking할 경우에도 예외적으로 처리합니다.
			Editor->GetGizmo()->ApplyScreenSpaceScaling(Camera->GetWorldLocation(), Camera->IsOrthogonal());

			Collector.CollectWorld(World, ShowFlags, ViewMode, Bus);
			Collector.CollectGrid(Settings.GridSpacing, Settings.GridHalfLineCount, Bus);
			Collector.CollectGizmo(Editor->GetGizmo(), ShowFlags, Bus);
			Collector.CollectSelection(
				Editor->GetSelectionManager().GetSelectedActors(),
				ShowFlags, ViewMode, Bus);

			Renderer.PrepareBatchers(Bus);
			// 1) Selection Mask RT에 선택된 오브젝트만 흰색으로 렌더
			Renderer.BeginFrame(Viewport->GetSelectionMaskRenderTarget());
			Renderer.RenderSinglePass(Bus, ERenderPass::StencilMask);
			Renderer.EndViewportFrame(Viewport->GetSelectionMaskRenderTarget());

			// 2) 일반 Scene RT 렌더
			Renderer.BeginFrame(Viewport->GetRenderTarget());
			Renderer.RenderScene(Bus);
			Renderer.EndViewportFrame(Viewport->GetRenderTarget());
		}
	}

	Bus.Clear();

	Renderer.BeginFrame();
	for (FViewport* Viewport : FViewportManager::Get().GetViewports())
	{
		Renderer.GetViewportRenderer().Render(Renderer.GetFD3DDevice().GetDeviceContext(), Viewport);
	}

	Renderer.SetDefaultViewport(); // TODO: Render Pass 수정 필요

	Collector.CollectFromSlate(Bus);
	Renderer.PrepareBatchers(Bus);

	Renderer.Render(Bus);
	Editor->RenderUI(DeltaTime);

	Renderer.EndFrame();
}
