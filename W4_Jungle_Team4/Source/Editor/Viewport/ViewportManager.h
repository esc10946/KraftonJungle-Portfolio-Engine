#pragma once

#include "Core/Singleton.h"
#include "Core/EngineTypes.h"

class FD3DDevice;
class FViewport;
class UWorld;
class FSelectionManager;
class FEditorViewportClient;
class SWidget;
class SViewport;
class UCameraComponent;

class FViewportManager : public TSingleton<FViewportManager>
{
	friend class TSingleton<FViewportManager>;
public:
	void Initialize(FD3DDevice* InDevice, UWorld* InWorld, FSelectionManager* InSelectionManager);
	void SetWorld(UWorld* InWorld);

	FViewport* CreateViewport();
	void DestroyViewport(FViewport* InViewport);

	FEditorViewportClient* CreateEditorViewportClient() const;

	TArray<FViewport*>& GetViewports() { return Viewports; }

	SViewport* GetFocusedViewportWidget() const { return FocusedViewportWidget; }
	void SetFocusedViewportWidget(SViewport* InViewportWidget) { FocusedViewportWidget = InViewportWidget; }

	UCameraComponent* GetFocusedCamera();
	void SetFocusedCamera(UCameraComponent* InCamera);

private:
	FViewportManager() = default;
	~FViewportManager() = default;

	FD3DDevice* Device = nullptr;
	UWorld* World = nullptr;
	FSelectionManager* SelectionManager = nullptr;

	TArray<FViewport*> Viewports;

	SWidget* ViewportRootWidget = nullptr;
	SViewport* FocusedViewportWidget = nullptr;

	UCameraComponent* FocusedCamera = nullptr;
};
