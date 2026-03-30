#pragma once

#include "Viewport/ViewportClient.h"
#include "Viewport/Viewport.h"
#include "Viewport/ViewOption.h"

#include "Render/Common/RenderTypes.h"
#include "Render/Common/ViewTypes.h"
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
	void OnViewportResized(uint32 NewTopLeftX, uint32 NewTopLeftY, uint32 NewWidth, uint32 NewHeight) override;

	void OnMouseDown(float X, float Y, int32 Button) override;
	void OnMouseUp(float X, float Y, int32 Button) override;
	void OnMouseMove(float X, float Y) override;
	void OnMouseWheel(float Delta) override;

	void OnKeyDown(int32 Key) override;
	void OnKeyUp(int32 Key) override;

	void ClearMovementInput();

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
	void SetCamera(UCameraComponent* InCamera) { Camera = InCamera; }

	// View Options (similar to UE)
	FViewOption GetViewOption(EViewOption Option) const { return ViewOptions[static_cast<uint32>(Option)]; }
	void SetViewOption(EViewOption Option, const FViewOption& ViewOption) { ViewOptions[static_cast<uint32>(Option)] = ViewOption; }

	FViewOption GetCurrentViewOption() const { return ViewOptions[static_cast<uint32>(CurrentViewOption)]; }
	void SetCurrentViewOption(EViewOption NewOption);

	void CreateDefaultViewOptions();
	void ApplyCurrentViewOption();

private:
	void MovePerspective(float DeltaTime);
	void RotatePerspective(float DeltaTime);
	void MoveOrthographic(float DeltaTime);
	void Zoom(float DeltaTime);

	void HandleDragStart(const FRay& Ray);

private:
	FViewport* Viewport = nullptr;

	UWorld* World = nullptr;
	UCameraComponent* Camera = nullptr;
	UGizmoComponent* Gizmo = nullptr;
	const FEditorSettings* Settings = nullptr;

	FSelectionManager* SelectionManager = nullptr;

	FViewOption ViewOptions[static_cast<uint32>(EViewOption::Max)] = {};
	EViewOption CurrentViewOption = EViewOption::Perspective;

	bool bIsCursorVisible = true;

	bool bIsLeftMouseDown = false;
	bool bIsRightMouseDown = false;

	bool bIsWDown = false; bool bIsADown = false; bool bIsSDown = false; bool bIsDDown = false;
	bool bIsQDown = false; bool bIsEDown = false;

	float MouseX = 0.0f; float MouseY = 0.0f;
	float MouseWheelDelta = 0.0f;
};
