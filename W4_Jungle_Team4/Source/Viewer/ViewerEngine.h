#pragma once
#include "Engine/Runtime/Engine.h"
#include "Viewer/ViewerViewportClient.h"


class UViewerEngine : public UEngine
{
public:

	// 생명주기
	virtual void Init(FWindowsWindow* InWindow) override; 
	virtual void Shutdown() override; 
	virtual void Tick(float DeltaTime) override; 
	virtual void OnWindowResized(uint32 Width, uint32 Height) override;

	// 모델
	bool LoadModel(const FString& ObjPath);
	void UnLoadModel();

private:
	FViewerViewportClient ViewerClient;
};