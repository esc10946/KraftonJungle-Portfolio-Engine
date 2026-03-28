#include "Editor/Viewport/ViewerViewportClient.h"

#include "Asset/StaticMesh/StaticMeshManager.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"

void FViewerViewportClient::LoadStaticMeshAsset(const std::string& FilePath)
{
	// static mesh asset Load
	CurrentMesh = FStaticMeshManager::Get().LoadStaticMeshAsset(FilePath);
}
