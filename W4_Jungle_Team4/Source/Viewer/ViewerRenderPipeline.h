#pragma once
#include "Engine/Render/Renderer/IRenderPipeline.h"
#include "Render/Scene/RenderBus.h"

class UViewerEngine;
class FViewerViewportClient;
class FMeshBuffer;

class FViewerRendererPipline : public IRenderPipeline
{
public:
	FViewerRendererPipline(FViewerViewportClient* InViewportClient, UViewerEngine* InViewer) 
		: ViewportClient(InViewportClient), Viewer(InViewer) {}
	~FViewerRendererPipline() = default;

	virtual void Execute(float DeltaTime, FRenderer& Renderer) override;

	void SetViewMode(EViewMode InViewMode) { ViewMode = InViewMode; }
	void SetShowFlags(const FShowFlags& InShowFlags) { ShowFlags = InShowFlags; }

private:
	FViewerViewportClient* ViewportClient = nullptr;
	UViewerEngine* Viewer = nullptr;
	FRenderBus Bus;

	EViewMode  ViewMode = EViewMode::Lit;
	FShowFlags ShowFlags = {};
};