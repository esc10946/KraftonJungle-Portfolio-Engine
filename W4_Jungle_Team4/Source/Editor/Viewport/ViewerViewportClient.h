#pragma once
#include "Viewport/FViewportClient.h"

class UOrbitCameraComponent;
struct FStaticMeshAsset;

class FViewerViewportClient : public FViewportClient
{
public:
	FViewerViewportClient() = default;
	~FViewerViewportClient() = default;

	void CreateCamera();
	void DestroyCamera();
	void ResetCamera();

	virtual void Tick(float DeltaTime) override;
	virtual void Draw(FViewport* Viewport, struct ID3D11DeviceContext* DeviceContext) override;
	virtual void OnViewportResized(FViewport* Viewport, uint32 NewWidth, uint32 NewHeight) override;

	void LoadOBJ(const std::string& FilePath);

private:
	FStaticMeshAsset* CurrentMesh = nullptr;
	UOrbitCameraComponent* ViewerCamera = nullptr;
};
