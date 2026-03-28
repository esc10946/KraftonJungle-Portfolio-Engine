#include "ViewerEngine.h"


void UViewerEngine::Init(FWindowsWindow* InWindow)
{
	// 엔진 공통 초기화
	UEngine::Init(InWindow);

	// 뷰어 전용 초기화
	ViewerClient.CreateCamera();
	ViewerClient.ResetCamera();


}

void UViewerEngine::Shutdown()
{

}

void UViewerEngine::Tick(float DeltaTime)
{

}

void UViewerEngine::OnWindowResized(uint32 Width, uint32 Height)
{
}

bool UViewerEngine::LoadModel(const FString& ObjPath)
{
	return false;
}

void UViewerEngine::UnLoadModel()
{
}
