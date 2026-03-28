#include "ViewerEngine.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "ViewerRenderPipeline.h"


void UViewerEngine::Init(FWindowsWindow* InWindow)
{
	// 엔진 공통 초기화
	UEngine::Init(InWindow);

	// 뷰어 전용 초기화
	ViewerClient.CreateCamera();
	ViewerClient.OnViewportResized(0, 0, InWindow->GetWidth(), InWindow->GetHeight());

	SetRenderPipeline(std::make_unique<FViewerRendererPipline>(&ViewerClient));
}

void UViewerEngine::Shutdown()
{
	ViewerClient.DestroyCamera();
	UEngine::Shutdown();
}

void UViewerEngine::Tick(float DeltaTime)
{
	UEngine::Tick(DeltaTime);
}

void UViewerEngine::OnWindowResized(uint32 Width, uint32 Height)
{
	UEngine::OnWindowResized(Width, Height);
	ViewerClient.OnViewportResized(0,0,Width, Height);
}

bool UViewerEngine::LoadModel(const FString& ObjPath)
{
	return false;
}

void UViewerEngine::UnLoadModel()
{
}
