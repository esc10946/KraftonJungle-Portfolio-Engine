#include "Asset/StaticMesh/StaticMeshObject.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"

#include <filesystem>

DEFINE_CLASS(UStaticMesh, UObject)
REGISTER_FACTORY(UStaticMesh)

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
