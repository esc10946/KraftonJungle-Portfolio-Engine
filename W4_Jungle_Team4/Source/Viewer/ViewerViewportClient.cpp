#include "Viewer/ViewerViewportClient.h"
#include "Engine/Core/InputSystem.h"
#include "Engine/Component/OrbitCameraComponent.h"
#include "Asset/StaticMesh/StaticMeshManager.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"

void FViewerViewportClient::CreateCamera()
{
	DestroyCamera();
	ViewerCamera = UObjectManager::Get().CreateObject<UOrbitCameraComponent>();
	ResetCamera();
}

void FViewerViewportClient::DestroyCamera()
{
	if (ViewerCamera)
	{
		UObjectManager::Get().DestroyObject(ViewerCamera);
		ViewerCamera = nullptr;
	}
}

void FViewerViewportClient::ResetCamera()
{
	if (!ViewerCamera) return;
	ViewerCamera->SetWorldLocation(FVector(0, 5, 0));
	ViewerCamera->LookAt(FVector(0,0,0));
}

void FViewerViewportClient::Tick(float DeltaTime)
{
	if (!ViewerCamera)
		return;

	if (InputSystem::Get().GetGuiInputState().bUsingKeyboard == true)
		return;

	if (InputSystem::Get().GetGuiInputState().bUsingMouse == true)
		return;

	float dx = InputSystem::Get().MouseDeltaX();
	float dy = InputSystem::Get().MouseDeltaY();

	if (InputSystem::Get().GetKey(VK_LBUTTON))
		ViewerCamera->OnOrbit(dx, dy);

	if (InputSystem::Get().GetKey(VK_RBUTTON))
		ViewerCamera->OnPan(dx, dy);

	float wheel = InputSystem::Get().GetScrollNotches();
	if (wheel != 0)
		ViewerCamera->OnZoom(wheel);
}

void FViewerViewportClient::OnViewportResized(uint32 NewTopLeftX, uint32 NewTopLeftY, uint32 NewWidth, uint32 NewHeight)
{
	if (NewWidth > 0 && NewHeight > 0)
	{
		WindowWidth = (float)NewWidth;
		WindowHeight = (float)NewHeight;
	}

	if (ViewerCamera)
		ViewerCamera->OnResize(NewWidth, NewHeight);
}

void FViewerViewportClient::LoadOBJ(const std::string& FilePath)
{
	CurrentMesh = FStaticMeshManager::Get().LoadStaticMeshAsset(FilePath);
}
