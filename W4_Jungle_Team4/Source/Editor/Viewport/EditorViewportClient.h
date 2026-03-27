#pragma once

#include "Viewport/ViewportClient.h"
#include "Viewport/Viewport.h"

#include "Render/Common/RenderTypes.h"
#include "Viewport/CursorOverlayState.h"
#include "Core/RayTypes.h"
#include "Core/CollisionTypes.h"

class UWorld;
class UCameraComponent;
class UGizmoComponent;
class FEditorSettings;
class FWindowsWindow;
class FSelectionManager;

class FEditorViewportClient : public FViewportClient
{
public:
	void Tick(float DeltaTime) override;
	void OnViewportResized(uint32 NewWidth, uint32 NewHeight) override;

	void Initialize(FViewport* InViewport);
	void SetWorld(UWorld* InWorld) { World = InWorld; }
	void SetGizmo(UGizmoComponent* InGizmo) { Gizmo = InGizmo; }
	void SetSettings(const FEditorSettings* InSettings) { Settings = InSettings; }
	void SetSelectionManager(FSelectionManager* InSelectionManager) { SelectionManager = InSelectionManager; }
	UGizmoComponent* GetGizmo() { return Gizmo; }

	// Camera lifecycle
	void CreateCamera();
	void DestroyCamera();
	void ResetCamera();
	UCameraComponent* GetCamera() const { return Camera; }

	const FCursorOverlayState& GetCursorOverlayState() const { return CursorOverlayState; }

	POINT ScreenToViewportPoint(POINT ScreenPoint) const;

private:
	void TickInput(float DeltaTime);
	void TickInteraction(float DeltaTime);
	void TickCursorOverlay(float DeltaTime);

	void HandleDragStart(const FRay& Ray);

private:
	FViewport* Viewport = nullptr;

	UWorld* World = nullptr;
	UCameraComponent* Camera = nullptr;
	UGizmoComponent* Gizmo = nullptr;
	const FEditorSettings* Settings = nullptr;

	FSelectionManager* SelectionManager = nullptr;

	FVector2 ViewportPos;
	FVector2 ViewportSize;

	bool bIsCursorVisible = true;

	FCursorOverlayState CursorOverlayState;
};
