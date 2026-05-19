#pragma once

#include "Core/CoreMinimal.h"

class FResourceManager;
class USkeletalMesh;
struct FSkeletalMesh;

class FSkeletalMeshLoadService
{
public:
	explicit FSkeletalMeshLoadService(FResourceManager& InResourceManager);

	// Load => binary
	USkeletalMesh* Load(const FString& Path);
	USkeletalMesh* Load(const FString& Path, const FString& SkeletonName);
	// Import => Source (Fbx)
	USkeletalMesh* ImportFbxSource(const FString& Path, const FString& SkeletonName);

private:
	USkeletalMesh* LoadBinary(const FString& BinaryPath, const FString& CacheKey);
	USkeletalMesh* LoadSiblingImportedBinary(const FString& NormalizedPath, const FString& SkeletonName);

	// FBX source import는 명시 import 경로에서만 허용.
	USkeletalMesh* ImportFbxSourceToBinary(const FString& NormalizedPath, const FString& SkeletonName);

	// 로드된 FSkeletalMesh 데이터 후처리:
	// material slot resolve → USkeletalMesh wrap → cache 등록.
	USkeletalMesh* FinalizeLoadedMesh(
		FSkeletalMesh* MeshData,
		const FString& ResolvePath,
		const FString& CacheKey,
		const FString& SkeletonReferenceBasePath);

	FResourceManager& ResourceManager;
};
