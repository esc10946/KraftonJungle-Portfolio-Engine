#include "Editor/Viewport/ViewerViewportClient.h"

#include "Asset/StaticMesh/StaticMeshManager.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"

void FViewerViewportClient::LoadOBJ(const std::string& FilePath)
{
	CurrentMesh = FStaticMeshManager::Get().LoadStaticMeshAsset(FilePath);
}
