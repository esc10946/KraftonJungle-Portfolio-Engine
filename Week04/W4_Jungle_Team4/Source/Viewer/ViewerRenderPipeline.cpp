#include "ViewerRenderPipeline.h"


#include "Engine/Render/Renderer/Renderer.h"
#include "Viewer/ViewerViewportClient.h"
#include "ViewerEngine.h"

void FViewerRendererPipline::Execute(float DeltaTime, FRenderer& Renderer)
{
	ViewportClient->Draw();
	Renderer.BeginFrame();
	Renderer.Render(ViewportClient->GetBus());
	OwnerEngine->RenderUI(DeltaTime);
	Renderer.EndFrame(); 
}
