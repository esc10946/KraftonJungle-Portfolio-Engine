#include "ViewerRenderPipeline.h"
#include "Engine/Render/Renderer/Renderer.h"
#include "Viewer/ViewerViewportClient.h"
#include "Engine/Component/OrbitCameraComponent.h"

void FViewerRendererPipline::Execute(float DeltaTime, FRenderer& Renderer)
{
	Bus.Clear();

	UOrbitCameraComponent* Camera = ViewportClient->GetCamera();
	if (!Camera) return;

	Bus.SetViewProjection(
		Camera->GetViewMatrix(),
		Camera->GetProjectionMatrix(),
		Camera->GetRightVector(),
		Camera->GetUpVector()
	);
	Bus.SetRenderSettings(ViewMode, ShowFlags);



	
	Renderer.BeginFrame();
	Renderer.Render(Bus);
	Renderer.EndFrame(); 
}
