#pragma once
#include "Engine/Runtime/Engine.h"
#include "Viewer/ViewerViewportClient.h"
#include "Viewer/ViewerPanel.h"


class FMeshBuffer;

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

	void RenderUI(float DeltaTime);

	FStaticMeshAsset* GetCurrentMesh() const { return CurrentMesh; }
	FTextureMeshBuffer* GetTextureMesh() { return &TextureMeshBuffer; }
	FVector GetCenter() { return Center; }
	float GetRadius() { return ModelRadius; }

	void BuildGpuBuffer(ID3D11Device* Device);

private:
	FViewerViewportClient ViewerClient;
	FViewerPanel Panel;

	FStaticMeshAsset* CurrentMesh = nullptr;
	FTextureMeshBuffer TextureMeshBuffer;

	FVector Center;
	float ModelSize;
	float ModelRadius;
};