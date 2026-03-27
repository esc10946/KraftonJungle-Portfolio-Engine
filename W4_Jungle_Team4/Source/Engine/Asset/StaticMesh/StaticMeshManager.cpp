#include "Asset/StaticMesh/StaticMeshManager.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/StaticMesh/StaticMeshBakeIO.h"
#include "AssetPipeline/Obj/ObjImporter.h"

#include <filesystem>

FStaticMeshAsset* FStaticMeshManager::LoadStaticMeshAsset(const FString& InSourcePath)
{
	// 같은 파일 경로 표기가 여러 방식일 수 있으므로 먼저 정규화
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);
	if (NormalizedPath.empty())
	{
		return nullptr;
	}

	// 1. 이미 메모리에 있으면 그대로 반환
	if (FStaticMeshAsset* FoundAsset = FindLoadedStaticMeshAsset(NormalizedPath))
	{
		return FoundAsset;
	}

	// 2. 아직 없으면 새 asset 생성
	FStaticMeshAsset* Asset = new FStaticMeshAsset();
	Asset->SourcePath = NormalizedPath;

	// 3. bake 경로 계산
	Asset->BakePath = FStaticMeshBakeIO::MakeBakePath(NormalizedPath);

	bool bLoaded = false;

	// 4. bake가 원본 obj보다 최신이면 bake 먼저 시도
	if (FStaticMeshBakeIO::IsBakeUpToDate(NormalizedPath, Asset->BakePath))
	{
		bLoaded = FStaticMeshBakeIO::LoadFromFile(Asset->BakePath, *Asset);
	}

	// 5. bake 로드에 실패했거나 bake가 없으면 obj를 다시 import
	if (!bLoaded)
	{
		bLoaded = FObjImporter::ImportFromFile(NormalizedPath, *Asset);

		// import 성공 시 bake도 같이 저장
		if (bLoaded)
		{
			Asset->BakePath = FStaticMeshBakeIO::MakeBakePath(NormalizedPath);
			FStaticMeshBakeIO::SaveToFile(Asset->BakePath, *Asset);
		}
	}

	// 6. 결국 실패하면 메모리 정리
	if (!bLoaded)
	{
		delete Asset;
		return nullptr;
	}

	// 7. 성공한 asset를 cache에 보관
	StaticMeshAssetMap[NormalizedPath] = Asset;
	return Asset;
}

FStaticMeshAsset* FStaticMeshManager::FindLoadedStaticMeshAsset(const FString& InSourcePath)
{
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);

	const auto It = StaticMeshAssetMap.find(NormalizedPath);
	if (It != StaticMeshAssetMap.end())
	{
		return It->second;
	}

	return nullptr;
}

void FStaticMeshManager::Clear()
{
	for (auto& Pair : StaticMeshAssetMap)
	{
		delete Pair.second;
	}

	StaticMeshAssetMap.clear();
}

FString FStaticMeshManager::NormalizeSourcePath(const FString& InSourcePath) const
{
	if (InSourcePath.empty())
	{
		return FString();
	}

	std::filesystem::path Path(InSourcePath);

	// 상대 경로면 절대 경로로 바꾸고 정규화
	Path = Path.is_absolute()
		? Path.lexically_normal()
		: std::filesystem::absolute(Path).lexically_normal();

	return Path.generic_string();
}
