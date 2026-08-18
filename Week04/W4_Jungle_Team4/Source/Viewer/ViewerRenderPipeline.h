#pragma once
#include "Engine/Render/Renderer/IRenderPipeline.h"
#include "Render/Scene/RenderBus.h"

class UViewerEngine;
class FViewerViewportClient;


class FViewerRendererPipline : public IRenderPipeline
{
public:
	FViewerRendererPipline(FViewerViewportClient* InViewportClient, UViewerEngine* InViewer) 
		: ViewportClient(InViewportClient), OwnerEngine(InViewer) {}
	~FViewerRendererPipline() = default;

	virtual void Execute(float DeltaTime, FRenderer& Renderer) override;

private:
	FViewerViewportClient* ViewportClient = nullptr;
	UViewerEngine* OwnerEngine = nullptr;
	
	bool bGPUReady = false;
};