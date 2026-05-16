#pragma once

#include "Core/Containers/String.h"
#include "Core/CoreTypes.h"

class FAssetPathPolicy
{
public:
	static bool FileExists(const FString& Path);
	static bool IsCurveAssetPath(const FString& Path);
	static bool IsAnimationClipAssetPath(const FString& Path);
	static bool IsSequenceAssetPath(const FString& Path);
	static bool IsSerializedMaterialAssetPath(const FString& Path);
	static FString MakeCookedStaticMeshBinaryPath(const FString& SourcePath);
	static FString MakeSiblingStaticMeshBinaryPath(const FString& SourcePath);
	static FString MakeStaticMeshCacheBinaryPath(const FString& SourcePath);
	static FString MakeWritableStaticMeshCacheBinaryPath(const FString& SourcePath);

	static FString SanitizeImportedAssetToken(const FString& Token);
	static FString MakeSiblingSkeletonBinaryPath(const FString& SourceFbxPath, const FString& SkeletonRootName);
	static FString MakeSiblingSkeletalMeshBinaryPath(const FString& SourceFbxPath, const FString& MeshOrSkeletonToken);
	static FString MakeSiblingAnimationClipBinaryPath(const FString& SourceFbxPath, const FString& ClipName);
	static FString MakeAssetRelativePath(const FString& FromAssetPath, const FString& ToAssetPath);
	static FString ResolveAssetRelativePath(const FString& FromAssetPath, const FString& RelativeTargetPath);

	static FString MakeWritableSkeletalMeshCacheBinaryPath(const FString& SourcePath);
	static FString MakeWritableAnimationClipCacheBinaryPath(const FString& SourcePath);
};
