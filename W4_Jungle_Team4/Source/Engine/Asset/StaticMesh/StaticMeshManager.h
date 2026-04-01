#pragma once

#include "Core/CoreTypes.h"
#include "Core/Singleton.h"

class UStaticMesh;
struct FStaticMeshAsset;

class FStaticMeshManager : public TSingleton<FStaticMeshManager>
{
	friend class TSingleton<FStaticMeshManager>;

public:
	// 경로를 받아 정적 메시 asset을 로드한다.
	// 순서:
	// 1) 메모리 cache 확인
	// 2) bake 파일 확인
	// 3) bake load 또는 obj import
	FStaticMeshAsset* LoadStaticMeshAsset(const FString& InSourcePath);

	// 이미 로드된 asset가 있으면 반환하고, 없으면 nullptr
	FStaticMeshAsset* FindLoadedStaticMeshAsset(const FString& InSourcePath);

	// asset 기반 UObject wrapper(UStaticMesh) 로드
	UStaticMesh* LoadStaticMeshObject(const FString& InSourcePath);
	UStaticMesh* FindLoadedStaticMeshObject(const FString& InSourcePath);

	// 여러 경로 미리 로드
	void Preload(const TArray<FString>& InSourcePaths);

	void Invalidate(const FString& InSourcePath);
	
	// manager가 가진 asset cache 정리
	void Clear();

private:
	FStaticMeshManager() = default;

	// 경로 문자열을 캐시 key로 쓰기 전에 정규화
	FString NormalizeSourcePath(const FString& InSourcePath) const;

private:
	TMap<FString, FStaticMeshAsset*> StaticMeshAssetMap;
	TMap<FString, UStaticMesh*> StaticMeshObjectMap;
};
