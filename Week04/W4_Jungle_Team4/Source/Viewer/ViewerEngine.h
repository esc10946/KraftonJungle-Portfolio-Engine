#pragma once
#include "Engine/Runtime/Engine.h"
#include "Viewer/ViewerViewportClient.h"
#include "Viewer/ViewerPanel.h"


class FMeshBuffer;

class UViewerEngine : public UEngine
{
public:
	DECLARE_CLASS(UViewerEngine, UEngine)
	virtual void Init(FWindowsWindow* InWindow) override; 
	virtual void Shutdown() override; 
	virtual void Tick(float DeltaTime) override; 
	virtual void OnWindowResized(uint32 Width, uint32 Height) override;

	bool LoadModel(const FString& ObjPath);

	void RenderUI(float DeltaTime);
	void CheckExternalFile();

private:
	FViewerViewportClient ViewerClient;
	FViewerPanel Panel;

	bool bExternalFileChecked = false;
};