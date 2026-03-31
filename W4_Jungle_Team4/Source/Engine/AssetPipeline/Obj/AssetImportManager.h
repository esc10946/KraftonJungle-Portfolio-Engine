#pragma once

#include "Core/CoreTypes.h"
#include <fstream>

struct FImportedObjPackage
{
	FString ImportedObjPath;
	FString ImportedMtlPath;
	TArray<FString> ImportedTexturePaths;

	bool IsValid() const
	{
		return !ImportedObjPath.empty();
	}
};

class FAssetImportManager
{
public:
	// 외부 obj 하나를 받아서
	// - Asset/Mesh 쪽으로 obj, mtl 복사
	// - Asset/Texture 쪽으로 png 복사
	// - 복사된 mtl의 map_Kd 경로를 새 경로로 rewrite
	// - 최종적으로 "프로젝트 내부 obj 경로"를 반환
	static bool ImportObjPackage(const FString& InSourceObjPath, FImportedObjPackage& OutPackage);

private:
	static FString MakeImportAssetName(const FString& InSourceObjPath);

	static bool ReadTextFile(const std::filesystem::path& InPath, FString& OutText);
	static bool WriteTextFile(const std::filesystem::path& InPath, const FString& InText);

	static bool CopyFileTo(const std::filesystem::path& InSourcePath, const std::filesystem::path& InDestPath);
	static bool EnsureDirectoryForFile(const std::filesystem::path& InFilePath);

	static FString RewriteMtlTexturePaths(
		const FString& InOriginalMtlText,
		const std::filesystem::path& InImportedMtlPath,
		const std::filesystem::path& InImportedTextureDir);

	static void CollectTextureSourcePaths(
		const std::filesystem::path& InSourceMtlPath,
		TArray<FString>& OutTextureSourcePaths);
};