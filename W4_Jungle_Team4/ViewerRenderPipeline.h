#pragma once
#include "Engine/Render/Renderer/IRenderPipeline.h"
#include "Render/Scene/RenderBus.h"

class FViewerViewportClient;
class FMeshBuffer;

class FViewerRendererPipline : public IRenderPipeline
{
public:
	FViewerRendererPipline(FViewerViewportClient* InViewportClient) : ViewportClient(InViewportClient) {}
	~FViewerRendererPipline() = default;

	virtual void Execute(float DeltaTime, FRenderer& Renderer) override;

	void SetViewMode(EViewMode InViewMode) { ViewMode = InViewMode; }
	void SetShowFlags(const FShowFlags& InShowFlags) { ShowFlags = InShowFlags; }

private:
	FViewerViewportClient* ViewportClient = nullptr;
	FRenderBus Bus;

	EViewMode  ViewMode = EViewMode::Lit;
	FShowFlags ShowFlags = {};
};