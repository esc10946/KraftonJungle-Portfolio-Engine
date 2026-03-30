#include "Asset/StaticMesh/StaticMeshManager.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/StaticMesh/StaticMeshBakeIO.h"
#include "AssetPipeline/Obj/ObjImporter.h"
#include "Asset/StaticMesh/StaticMeshObject.h"

#include <filesystem>

#include <iostream> // opt) test

FStaticMeshAsset* FStaticMeshManager::LoadStaticMeshAsset(const FString& InSourcePath)
{
	// opt) 함수 시작 부분
	std::cout << "[StaticMeshManager] request load: " << InSourcePath << '\n';

	// 같은 파일 경로 표기가 여러 방식일 수 있으므로 먼저 정규화
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);
	if (NormalizedPath.empty())
	{
		return nullptr;
	}

	// 1. 이미 메모리에 있으면 그대로 반환
	if (FStaticMeshAsset* FoundAsset = FindLoadedStaticMeshAsset(NormalizedPath))
	{
		// opt) 재로드, 캐싱 확인
		std::cout
			<< "[StaticMeshManager] already loaded, reuse cached asset: "
			<< NormalizedPath
			<< " (vertices=" << FoundAsset->Vertices.size()
			<< ", indices=" << FoundAsset->Indices.size()
			<< ", sections=" << FoundAsset->Sections.size()
			<< ")\n";
		return FoundAsset;
	}

	// 2. 아직 없으면 새 asset 생성
	FStaticMeshAsset* Asset = new FStaticMeshAsset();
	Asset->SourcePath = NormalizedPath;

	// 3. bake 경로 계산
	Asset->BakePath = FStaticMeshBakeIO::MakeBakePath(NormalizedPath);


	// opt) test bake path 계산 직후
	std::cout
		<< "[StaticMeshManager] normalized path: " << NormalizedPath << '\n'
		<< "[StaticMeshManager] bake path: " << Asset->BakePath << '\n';

	bool bLoaded = false;

	// 4. bake가 원본 obj보다 최신이면 bake 먼저 시도
	// bake 유효성 판단
	if (FStaticMeshBakeIO::IsBakeUpToDate(NormalizedPath, Asset->BakePath))
	{
		// .smbin 바이너리 파일에서 FStaticMeshAsset 복원 함수
		bLoaded = FStaticMeshBakeIO::LoadFromFile(Asset->BakePath, *Asset);
		if (bLoaded) // bake load 성공 시
		{
			std::cout << "[StaticMeshManager] loaded from bake\n";
		}
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

			// opt) import 성공 시
			std::cout << "[StaticMeshManager] imported from obj and saved bake\n";
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

	// opt) 최종 반환 직전
	std::cout
		<< "[StaticMeshManager] done: vertices=" << Asset->Vertices.size()
		<< ", indices=" << Asset->Indices.size()
		<< ", sections=" << Asset->Sections.size()
		<< '\n';

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

UStaticMesh* FStaticMeshManager::LoadStaticMeshObject(const FString& InSourcePath)
{
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);
	if (NormalizedPath.empty())
	{
		return nullptr;
	}

	// 1. object cache 우선 확인
	if (UStaticMesh* FoundMesh = FindLoadedStaticMeshObject(NormalizedPath))
	{
		return FoundMesh;
	}

	// 2. asset 로드
	FStaticMeshAsset* Asset = LoadStaticMeshAsset(NormalizedPath);
	if (!Asset)
	{
		return nullptr;
	}

	// 3. UObject wrapper 생성
	UStaticMesh* MeshObject = ConstructObject<UStaticMesh>();
	MeshObject->SetStaticMeshAsset(Asset);

	StaticMeshObjectMap[NormalizedPath] = MeshObject;
	return MeshObject;
}


UStaticMesh* FStaticMeshManager::FindLoadedStaticMeshObject(const FString& InSourcePath)
{ // "이 경로의 UStaticMesh UObject를 이미 만들어둔 게 있으면 찾아서 재사용해라"
	// key = NoramlizedSourcePath
	const FString NormalizedPath = NormalizeSourcePath(InSourcePath);

	// 두 단계에 걸친 캐시 찾기
	// (1) 매니저가 기억하고 있는 빠른 캐시에서 찾기
	// (2) 매니저 캐시는 비어있지만, UObject 자체는 이미 살아있을 수 있으니 전역 객체 목록에서 복구
	// 매니저 Clear()는 StaticMeshObjectMap.Clear()만 하고, UStaticMesh 객체 자체를 destroy하지는 않음. 
	// 그래서 "맵은 비었는지 UObject는 살아 있는 상태"가 가능. 그걸 다시 찾아 캐시에 넣으려는 것.
	// QQQ. 근데 이거까지 굳이굳이 해야하는건가?
	
	// 1. object cache 확인
	// map에서 NormalizedPath key 찾기. 
	const auto CacheIt = StaticMeshObjectMap.find(NormalizedPath);
	if (CacheIt != StaticMeshObjectMap.end()) // 찾음.
	{
		return CacheIt->second; // Value
	}

	// 2. 이미 생성된 UObject 전체 순회
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

	// 상대 경로면 절대 경로로 바꾸고 정규화
	Path = Path.is_absolute()
		? Path.lexically_normal()
		: std::filesystem::absolute(Path).lexically_normal();

	return Path.generic_string();
}
