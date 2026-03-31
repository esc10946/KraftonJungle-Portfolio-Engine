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


bool FViewerViewportClient::LoadOBJ(const FString& ObjPath)
{
	FStaticMeshAsset* NewMesh = FStaticMeshManager::Get().LoadStaticMeshAsset(ObjPath);

	if (!NewMesh) return false;

	if (NewMesh->Vertices.empty()) return false;

	NewMesh->UpdateBounds();

	UnLoadOBJ();
	CurrentMesh = NewMesh;
	FVector Min = CurrentMesh->BoundsMin;
	FVector Max = CurrentMesh->BoundsMax;

	Center.X = (Min.X + Max.X) * 0.5f;
	Center.Y = (Min.Y + Max.Y) * 0.5f;
	Center.Z = (Min.Z + Max.Z) * 0.5f;

	FVector Diff = Max - Min;
	ModelSize = Diff.Length();
	ModelRadius = ModelSize * 0.5f;

	ViewerCamera->SetTarget(Center);
	ViewerCamera->SetRadius(ModelRadius * 2.5f);
	ViewerCamera->UpdatePosition();
	ViewerCamera->SetMinRadius(ModelRadius * 1.1f);
	ViewerCamera->SetMaxRadius(ModelRadius * 10.0f);

	return true;
}

void FViewerViewportClient::UnLoadOBJ()
{
	if (TextureMeshBuffer.IsValid())
		TextureMeshBuffer.Release();

	if (CurrentMesh)
	{
		CurrentMesh = nullptr;
	}
}

void FViewerViewportClient::BuildGpuBuffer()
{
	if (TextureMeshBuffer.IsValid() || !CurrentMesh) return;

	if (!Renderer || Renderer->GetFD3DDevice().GetDevice() == nullptr) return;

	FTexturedMeshData MeshData;
	if (CurrentMesh->BuildTexturedRenderMeshData(MeshData))
	{
		TextureMeshBuffer.Create(Renderer->GetFD3DDevice().GetDevice(), MeshData);
	}
}

void FViewerViewportClient::Draw()
{
	PrepareRenderBus();

	if (CurrentMesh)
	{
		BuildGpuBuffer();
		GenerateStaticMeshCommands();
	}
}

void FViewerViewportClient::Tick(float DeltaTime)
{
	if (!ViewerCamera || IgnoreInput())
		return;

	float dx, dy;
	GetSafeMouseDelta(dx, dy);

	if (InputSystem::Get().GetKey(VK_LBUTTON))
		ViewerCamera->OnOrbit(dx, dy);

	if (InputSystem::Get().GetKey(VK_RBUTTON))
		ViewerCamera->OnPan(dx, dy, ModelRadius);

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

void FViewerViewportClient::GenerateStaticMeshCommands()
{
	if (!TextureMeshBuffer.IsValid() || !CurrentMesh) return;

	for (const FStaticMeshSection& Section : CurrentMesh->Sections)
	{
		FRenderCommand Cmd = CreateCommand(Section);
		Bus.AddCommand(ERenderPass::Opaque, Cmd);
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
		float RawX = static_cast<float>(InputSystem::Get().MouseDeltaX());
        float RawY = static_cast<float>(InputSystem::Get().MouseDeltaY());
        
        SmoothedDeltaX = SmoothedDeltaX * 0.3f + RawX * 0.3f;
        SmoothedDeltaY = SmoothedDeltaY * 0.3f + RawY * 0.3f;
        
        outX = SmoothedDeltaX;
        outY = SmoothedDeltaY;
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

FRenderCommand FViewerViewportClient::CreateCommand(const FStaticMeshSection& Section)
{
	FRenderCommand Cmd = {};
	Cmd.Type = ERenderCommandType::StaticMesh;
	Cmd.TextureMeshBuffer = &TextureMeshBuffer;
	Cmd.PerObjectConstants.Model = FMatrix::Identity;
	Cmd.MeshRenderData.FirstIndex = Section.FirstIndex;
	Cmd.MeshRenderData.IndexCount = Section.IndexCount;

	if (Section.MaterialSlotIndex < CurrentMesh->MaterialSlots.size())
	{
		const FString& TexturePath = CurrentMesh->MaterialSlots[Section.MaterialSlotIndex].DiffuseTexturePath;
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





