#pragma once
#include "Editor/Viewport/ViewportClient.h"
#include "Engine/Render/Scene/RenderBus.h"

class UOrbitCameraComponent;
struct FStaticMeshAsset;
class UViewerEngine;
class FRenderer;
struct FStaticMeshSection;

class FViewerViewportClient : public FViewportClient
{
public:
	FViewerViewportClient() = default;
	~FViewerViewportClient() = default;

	void CreateCamera();
	void DestroyCamera();
	void ResetCamera();
	UOrbitCameraComponent* GetCamera() const { return ViewerCamera; }

	void Draw();
	virtual void Tick(float DeltaTime) override;
	virtual void OnViewportResized(uint32 NewTopLeftX, uint32 NewTopLeftY, uint32 NewWidth, uint32 NewHeight) override;

	virtual void OnMouseDown(float X, float Y, int32 Button){}
	virtual void OnMouseUp(float X, float Y, int32 Button) {}
	virtual void OnMouseMove(float X, float Y) {}
	virtual void OnMouseWheel(float Delta) {}

	virtual void OnKeyDown(int32 Key) {}
	virtual void OnKeyUp(int32 Key) {}

	void SetEngine(UViewerEngine* InEngine) { OwnerEngine = InEngine; }
	void SetRenderer(FRenderer* InRenderer) { Renderer = InRenderer; }
	void SetViewMode(EViewMode InViewMode) { ViewMode = InViewMode; }
	void SetShowFlags(const FShowFlags& InShowFlags) { ShowFlags = InShowFlags; }

	FRenderBus& GetBus() { return Bus; }

	void PrepareRenderBus();
	void EnsureGpuResources(FStaticMeshAsset* Asset);
	void GenerateStaticMeshCommands(FStaticMeshAsset* Asset);

	void CheckAssetSystem();
	bool IgnoreInput();
	void GetSafeMouseDelta(float& outX, float& outY);
	bool IsMouseInsideWindow();

private:
	FRenderCommand CreateCommand(const FStaticMeshSection& Section, FStaticMeshAsset* Asset);

	UOrbitCameraComponent* ViewerCamera = nullptr;
	UViewerEngine* OwnerEngine = nullptr;
	FRenderer* Renderer = nullptr;
	
	float WindowWidth = 1920.f;
	float WindowHeight = 1080.f;

	FRenderBus Bus;
	EViewMode  ViewMode = EViewMode::Lit;
	FShowFlags ShowFlags = {};
};
