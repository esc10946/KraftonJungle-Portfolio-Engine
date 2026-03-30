#include "Viewer/ViewerViewportClient.h"
#include "Engine/Core/InputSystem.h"
#include "Engine/Component/OrbitCameraComponent.h"
#include "Asset/StaticMesh/StaticMeshManager.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "ViewerEngine.h"
#include "Engine/Core/ResourceManager.h"
#include "Asset/Material/MaterialManager.h"
#include "Engine/Runtime/WindowsWindow.h"

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


void FViewerViewportClient::Draw()
{
	PrepareRenderBus();
	FStaticMeshAsset* Asset = OwnerEngine->GetCurrentMesh();

	if (Asset)
	{
		EnsureGpuResources(Asset);
		GenerateStaticMeshCommands(Asset);
	}
}

void FViewerViewportClient::Tick(float DeltaTime)
{
	if (!ViewerCamera || IgnoreInput())
		return;

	CheckAssetSystem();

	float dx, dy;
	GetSafeMouseDelta(dx, dy);

	if (InputSystem::Get().GetKey(VK_LBUTTON))
		ViewerCamera->OnOrbit(dx, dy);

	if (InputSystem::Get().GetKey(VK_RBUTTON))
		ViewerCamera->OnPan(dx, dy, OwnerEngine->GetRadius());

	float wheel = InputSystem::Get().GetScrollNotches();
	if (wheel != 0)
		ViewerCamera->OnZoom(wheel);

	Draw();
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

void FViewerViewportClient::PrepareRenderBus()
{
	Bus.Clear();
	
	if (!ViewerCamera) return;

	Bus.SetViewProjection(
		ViewerCamera->GetViewMatrix(),
		ViewerCamera->GetProjectionMatrix(),
		ViewerCamera->GetRightVector(),
		ViewerCamera->GetUpVector()
	);
	Bus.SetRenderSettings(ViewMode, ShowFlags);
}

void FViewerViewportClient::EnsureGpuResources(FStaticMeshAsset* Asset)
{
	if (!Asset) return;

	if (!Renderer || Renderer->GetFD3DDevice().GetDevice() == nullptr) return;

	OwnerEngine->BuildGpuBuffer(Renderer->GetFD3DDevice().GetDevice());
}

void FViewerViewportClient::GenerateStaticMeshCommands(FStaticMeshAsset* Asset)
{
	if (!OwnerEngine->GetTextureMesh() || !Asset) return;

	for (const FStaticMeshSection& Section : Asset->Sections)
	{
		FRenderCommand Cmd = CreateCommand(Section, Asset);
		Bus.AddCommand(ERenderPass::Opaque, Cmd);
	}
}

void FViewerViewportClient::CheckAssetSystem()
{
	static FStaticMeshAsset* LastAsset = nullptr;
	FStaticMeshAsset* CurrentAsset = OwnerEngine->GetCurrentMesh();

	if (CurrentAsset != LastAsset)
	{
		if (CurrentAsset)
		{
			FVector NewCenter = OwnerEngine->GetCenter();
			float NewRadius = OwnerEngine->GetRadius();

			ViewerCamera->SetTarget(NewCenter);
			ViewerCamera->SetRadius(NewRadius * 2.5f);

			ViewerCamera->UpdatePosition();
		}

		LastAsset = CurrentAsset;
	}
}

bool FViewerViewportClient::IgnoreInput()
{
	if (InputSystem::Get().GetGuiInputState().bUsingKeyboard
		|| InputSystem::Get().GetGuiInputState().bUsingMouse)
		return true;

	return false;
}

void FViewerViewportClient::GetSafeMouseDelta(float& outX, float& outY)
{
	outX = 0;
	outY = 0;

	if (IsMouseInsideWindow())
	{
		outX = InputSystem::Get().MouseDeltaX();
		outY = InputSystem::Get().MouseDeltaY();
	}
}

bool FViewerViewportClient::IsMouseInsideWindow()
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient((HWND)OwnerEngine->GetWindow()->GetHWND(), &pt);

	RECT rc;
	GetClientRect((HWND)OwnerEngine->GetWindow()->GetHWND(), &rc);

	return (pt.x >= 0 && pt.x <= rc.right && pt.y >= 0 && pt.y <= rc.bottom);
}

FRenderCommand FViewerViewportClient::CreateCommand(const FStaticMeshSection& Section, FStaticMeshAsset* Asset)
{
	FRenderCommand Cmd = {};
	Cmd.Type = ERenderCommandType::StaticMesh;
	Cmd.TextureMeshBuffer = OwnerEngine->GetTextureMesh();
	Cmd.PerObjectConstants.Model = FMatrix::Identity;
	Cmd.MeshRenderData.FirstIndex = Section.FirstIndex;
	Cmd.MeshRenderData.IndexCount = Section.IndexCount;

	if (Section.MaterialSlotIndex < Asset->MaterialSlots.size())
	{
		const FString& TexturePath = Asset->MaterialSlots[Section.MaterialSlotIndex].DiffuseTexturePath;
		Cmd.MeshRenderData.DiffuseSRV = FResourceManager::Get().GetOrCreateTextureSRV(
			TexturePath,
			Renderer->GetFD3DDevice().GetDevice()
		);

		if (Cmd.MeshRenderData.DiffuseSRV != nullptr)
		{
			Cmd.PerObjectConstants.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
			Cmd.Constants.StaticMesh.StaticMeshUseTexture = 1;
		}
		else
		{
			Cmd.PerObjectConstants.Color = FVector4(0.2f, 0.2f, 0.2f, 1.0f);
			Cmd.Constants.StaticMesh.StaticMeshUseTexture = 0;
		}

		Cmd.Constants.StaticMesh.UVTiling[0] = 1.0f;
		Cmd.Constants.StaticMesh.UVTiling[1] = 1.0f;
		Cmd.Constants.StaticMesh.UVOffset[0] = 0.0f;
		Cmd.Constants.StaticMesh.UVOffset[1] = 0.0f;
		Cmd.Constants.StaticMesh.UVScrollSpeed[0] = 0.0f;
		Cmd.Constants.StaticMesh.UVScrollSpeed[1] = 0.0f;

		return Cmd;
	}
}





