#include "ViewerRenderPipeline.h"

#include <iostream>

#include "Engine/Render/Renderer/Renderer.h"
#include "Viewer/ViewerViewportClient.h"
#include "ViewerEngine.h"

void FViewerRendererPipline::Execute(float DeltaTime, FRenderer& Renderer)
{
	ViewportClient->Draw();

	Renderer.BeginFrame();
	FRenderBus& Bus = ViewportClient->GetBus();
	Renderer.Render(Bus);
	OwnerEngine->RenderUI(DeltaTime);
	Renderer.EndFrame(); 
}
