#pragma once
#include "Editor/Viewport/ViewportClient.h"

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
	UOrbitCameraComponent* GetCamera() const { return ViewerCamera; }
	const FStaticMeshAsset* GetCurrentMesh() const { return CurrentMesh; }

	virtual void Tick(float DeltaTime) override;
	virtual void OnViewportResized(uint32 NewTopLeftX, uint32 NewTopLeftY, uint32 NewWidth, uint32 NewHeight) override;

	virtual void OnMouseDown(float X, float Y, int32 Button){}
	virtual void OnMouseUp(float X, float Y, int32 Button) {}
	virtual void OnMouseMove(float X, float Y) {}
	virtual void OnMouseWheel(float Delta) {}

	virtual void OnKeyDown(int32 Key) {}
	virtual void OnKeyUp(int32 Key) {}

	void LoadOBJ(const std::string& FilePath);

private:
	FStaticMeshAsset* CurrentMesh = nullptr;
	UOrbitCameraComponent* ViewerCamera = nullptr;
	
	float WindowWidth = 1920.f;
	float WindowHeight = 1080.f;
};
