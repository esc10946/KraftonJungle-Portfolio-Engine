#include "Asset/StaticMesh/StaticMeshObject.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"

#include <filesystem>

DEFINE_CLASS(UStaticMesh, UObject)
REGISTER_FACTORY(UStaticMesh)

UStaticMesh::~UStaticMesh()
{  
	if (MeshBuffer)
    {
        MeshBuffer->Release();
    }
}

void UStaticMesh::SetStaticMeshAsset(FStaticMeshAsset* InStaticMeshAsset)
{
	StaticMeshAsset = InStaticMeshAsset;
}

const FString& UStaticMesh::GetAssetPathFileName() const
{
	static const FString EmptyPath;
	return StaticMeshAsset ? StaticMeshAsset->SourcePath : EmptyPath;
}

FString UStaticMesh::GetDisplayName() const
{
	const FString& AssetPath = GetAssetPathFileName();
	if (AssetPath.empty())
	{
		return GetFName().ToString();
	}

	return std::filesystem::path(AssetPath).filename().string();
}

FMeshBuffer* UStaticMesh::GetOrCreateMeshBuffer(ID3D11Device* Device)
{
	StaticMeshAsset->BuildRenderMeshData(RenderMeshData);

	MeshBuffer = std::make_unique<FMeshBuffer>();

	MeshBuffer->Create(Device, RenderMeshData);
	
	return MeshBuffer.get();
}

void UStaticMesh::ReleaseMeshBuffer()
{
}
