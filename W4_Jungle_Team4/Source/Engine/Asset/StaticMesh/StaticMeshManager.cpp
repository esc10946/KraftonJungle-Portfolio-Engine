#include "Asset/StaticMesh/StaticMeshManager.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/StaticMesh/StaticMeshBakeIO.h"
#include "Asset/StaticMesh/StaticMeshObject.h"
#include "Object/Object.h"

#include <filesystem>

namespace
{
	FStaticMeshVertex MakeTestVertex(const FVector& Position, const FVector4& Color)
	{
		FStaticMeshVertex Vertex;
		Vertex.Position = Position;
		Vertex.Normal = FVector(Position.X, Position.Y, 0.5f).Normalized();
		Vertex.Color = Color;
		Vertex.TexCoord = FVector2(0.0f, 0.0f);
		return Vertex;
	}
}

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

	bool bLoaded = BuildTestStaticMeshAsset(NormalizedPath, *Asset);
	if (!bLoaded && FStaticMeshBakeIO::IsBakeUpToDate(NormalizedPath, Asset->BakePath))
	{
		bLoaded = FStaticMeshBakeIO::LoadFromFile(Asset->BakePath, *Asset);
	}

	if (!bLoaded)
	{
		bLoaded;// = FObjImporter::ImportFromFile(NormalizedPath, *Asset);
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
	if (NormalizedPath == GetTestStaticMeshPath())
	{
		MeshObject->SetFName(FName("SM_TestPyramid"));
	}
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

FString FStaticMeshManager::GetTestStaticMeshPath() const
{
	return NormalizeSourcePath("Generated/TestPyramid.obj");
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

bool FStaticMeshManager::BuildTestStaticMeshAsset(const FString& InSourcePath, FStaticMeshAsset& OutAsset) const
{
	if (InSourcePath != GetTestStaticMeshPath())
	{
		return false;
	}

	OutAsset.Reset();
	OutAsset.SourcePath = InSourcePath;

	OutAsset.Vertices = {
		MakeTestVertex(FVector(-0.5f, -0.5f, 0.0f), FVector4(1.0f, 0.2f, 0.2f, 1.0f)),
		MakeTestVertex(FVector( 0.5f, -0.5f, 0.0f), FVector4(0.2f, 1.0f, 0.2f, 1.0f)),
		MakeTestVertex(FVector( 0.5f,  0.5f, 0.0f), FVector4(0.2f, 0.4f, 1.0f, 1.0f)),
		MakeTestVertex(FVector(-0.5f,  0.5f, 0.0f), FVector4(1.0f, 0.9f, 0.2f, 1.0f)),
		MakeTestVertex(FVector( 0.0f,  0.0f, 0.9f), FVector4(1.0f, 1.0f, 1.0f, 1.0f)),
	};

	OutAsset.Indices = {
		0, 1, 4,
		1, 2, 4,
		2, 3, 4,
		3, 0, 4,
		0, 2, 1,
		0, 3, 2,
	};

	FStaticMeshSection Section;
	Section.FirstIndex = 0;
	Section.IndexCount = static_cast<uint32>(OutAsset.Indices.size());
	Section.MaterialName = "Generated/Test";
	OutAsset.Sections.push_back(Section);
	OutAsset.UpdateBounds();
	return true;
}
