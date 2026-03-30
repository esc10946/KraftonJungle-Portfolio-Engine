#include "ViewerEngine.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "ViewerRenderPipeline.h"
#include "Engine/Asset/StaticMesh/StaticMeshManager.h"
#include "Engine/Asset/StaticMesh/StaticMeshAsset.h"


void UViewerEngine::Init(FWindowsWindow* InWindow)
{
	// 엔진 공통 초기화
	UEngine::Init(InWindow);

	FStaticMeshManager::Get();

	// 뷰어 전용 초기화
	ViewerClient.SetEngine(this);
	ViewerClient.SetRenderer(&Renderer);
	ViewerClient.CreateCamera();
	ViewerClient.OnViewportResized(0, 0, InWindow->GetWidth(), InWindow->GetHeight());

	// ImGui 초기화
	Panel.Create(Window, Renderer, this);
	Panel.SetViewportClient(&ViewerClient);

	SetRenderPipeline(std::make_unique<FViewerRendererPipline>(&ViewerClient, this));
}

void UViewerEngine::Shutdown()
{
	ViewerClient.DestroyCamera();
	Panel.Release();
	UEngine::Shutdown();
}

void UViewerEngine::Tick(float DeltaTime)
{
	UEngine::Tick(DeltaTime);
	ViewerClient.Tick(DeltaTime);
}

void UViewerEngine::OnWindowResized(uint32 Width, uint32 Height)
{
	UEngine::OnWindowResized(Width, Height);
	ViewerClient.OnViewportResized(0,0,Width, Height);
}

bool UViewerEngine::LoadModel(const FString& ObjPath)
{
	UnLoadModel();

	CurrentMesh = FStaticMeshManager::Get().LoadStaticMeshAsset(ObjPath);
	if (!CurrentMesh) return false;

	return true;
}

void UViewerEngine::UnLoadModel()
{
	TextureMeshBuffer.Release();

	if (CurrentMesh)
	{
		delete CurrentMesh;
		CurrentMesh = nullptr;
	}
}

void UViewerEngine::BuildGpuBuffer(ID3D11Device* Device)
{
	if (TextureMeshBuffer.IsValid() || !CurrentMesh) return;

	FTexturedMeshData MeshData;
	if (CurrentMesh->BuildTexturedRenderMeshData(MeshData))
	{
		TextureMeshBuffer.Create(Device, MeshData);
	}
}


void UViewerEngine::RenderUI(float DeltaTime)
{
	Panel.BeginFrame();
	Panel.Render(DeltaTime);
}