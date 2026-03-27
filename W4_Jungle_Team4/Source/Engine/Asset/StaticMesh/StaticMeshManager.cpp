#include "Asset/StaticMesh/StaticMeshManager.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/StaticMesh/StaticMeshBakeIO.h"
#include "Asset/StaticMesh/StaticMeshObject.h"
#include "AssetPipeline/Obj/ObjImporter.h"
#include "Object/Object.h"

#include <filesystem>

FStaticMeshAsset* FStaticMeshManager::LoadStaticMeshAsset(const FString& InSourcePath)
{
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);
	if (NormalizedPath.empty())
	{
		return nullptr;
	}

	if (FStaticMeshAsset* FoundAsset = FindLoadedStaticMeshAsset(NormalizedPath))
	{
		return FoundAsset;
	}

	FStaticMeshAsset* Asset = new FStaticMeshAsset();
	Asset->SourcePath = NormalizedPath;
	Asset->BakePath = FStaticMeshBakeIO::MakeBakePath(NormalizedPath);

	bool bLoaded = false;
	if (FStaticMeshBakeIO::IsBakeUpToDate(NormalizedPath, Asset->BakePath))
	{
		bLoaded = FStaticMeshBakeIO::LoadFromFile(Asset->BakePath, *Asset);
	}

	if (!bLoaded)
	{
		bLoaded = FObjImporter::ImportFromFile(NormalizedPath, *Asset);
		if (bLoaded)
		{
			Asset->BakePath = FStaticMeshBakeIO::MakeBakePath(NormalizedPath);
			FStaticMeshBakeIO::SaveToFile(Asset->BakePath, *Asset);
		}
	}

	if (!bLoaded)
	{
		delete Asset;
		return nullptr;
	}

	StaticMeshAssetMap[NormalizedPath] = Asset;
	return Asset;
}

UStaticMesh* FStaticMeshManager::LoadStaticMeshObject(const FString& InSourcePath)
{
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);
	if (NormalizedPath.empty())
	{
		return nullptr;
	}

	if (UStaticMesh* FoundMesh = FindLoadedStaticMeshObject(NormalizedPath))
	{
		return FoundMesh;
	}

	FStaticMeshAsset* Asset = LoadStaticMeshAsset(NormalizedPath);
	if (!Asset)
	{
		return nullptr;
	}

	UStaticMesh* MeshObject = ConstructObject<UStaticMesh>();
	MeshObject->SetStaticMeshAsset(Asset);
	StaticMeshObjectMap[NormalizedPath] = MeshObject;
	return MeshObject;
}

FStaticMeshAsset* FStaticMeshManager::FindLoadedStaticMeshAsset(const FString& InSourcePath)
{
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);
	const auto It = StaticMeshAssetMap.find(NormalizedPath);
	return (It != StaticMeshAssetMap.end()) ? It->second : nullptr;
}

UStaticMesh* FStaticMeshManager::FindLoadedStaticMeshObject(const FString& InSourcePath)
{
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);

	const auto CacheIt = StaticMeshObjectMap.find(NormalizedPath);
	if (CacheIt != StaticMeshObjectMap.end())
	{
		return CacheIt->second;
	}

	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* Mesh = *It;
		if (Mesh && Mesh->GetAssetPathFileName() == NormalizedPath)
		{
			StaticMeshObjectMap[NormalizedPath] = Mesh;
			return Mesh;
		}
	}

	return nullptr;
}

void FStaticMeshManager::Preload(const TArray<FString>& InSourcePaths)
{
	for (const FString& SourcePath : InSourcePaths)
	{
		LoadStaticMeshObject(SourcePath);
	}
}

void FStaticMeshManager::Clear()
{
	for (auto& Pair : StaticMeshAssetMap)
	{
		delete Pair.second;
	}

	StaticMeshAssetMap.clear();
	StaticMeshObjectMap.clear();
}

FString FStaticMeshManager::NormalizeSourcePath(const FString& InSourcePath) const
{
	if (InSourcePath.empty())
	{
		return FString();
	}

	std::filesystem::path Path(InSourcePath);
	Path = Path.is_absolute() ? Path.lexically_normal() : std::filesystem::absolute(Path).lexically_normal();
	return Path.generic_string();
}
