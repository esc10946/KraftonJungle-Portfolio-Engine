#include "Editor/Viewport/ViewerViewportClient.h"
#include "Engine/Core/InputSystem.h"
#include "Engine/Component/OrbitCameraComponent.h"
#include "Asset/StaticMesh/StaticMeshManager.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"

void FViewerViewportClient::CreateCamera()
{
	DestroyCamera();
	ViewerCamera = UObjectManager::Get().CreateObject<UOrbitCameraComponent>();
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
	{
		return;
	}

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

void FViewerViewportClient::Draw(FViewport* Viewport, ID3D11DeviceContext* DeviceContext)
{

}

void FViewerViewportClient::OnViewportResized(FViewport* Viewport, uint32 NewWidth, uint32 NewHeight)
{
	(void)Viewport;

	if (!ViewerCamera) return;
	ViewerCamera->OnResize(NewWidth, NewHeight);
}

void FViewerViewportClient::LoadOBJ(const std::string& FilePath)
{
	CurrentMesh = FStaticMeshManager::Get().LoadStaticMeshAsset(FilePath);
}
