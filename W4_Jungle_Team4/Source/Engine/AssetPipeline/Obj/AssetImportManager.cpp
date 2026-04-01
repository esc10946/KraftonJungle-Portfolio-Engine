#include "AssetPipeline/Obj/AssetImportManager.h"

#include "AssetPipeline/Obj/ObjParser.h"
#include "AssetPipeline/Obj/MtlParser.h"
#include "AssetPipeline/Obj/ObjTypes.h"
#include "Core/Paths.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace
{
	FString NormalizePathString(const std::filesystem::path& InPath)
	{
		return InPath.lexically_normal().generic_string();
	}

	FString SanitizeAssetName(const FString& InName)
	{
		FString Result = InName;
		for (char& Ch : Result)
		{
			const bool bValid =
				(Ch >= 'a' && Ch <= 'z') ||
				(Ch >= 'A' && Ch <= 'Z') ||
				(Ch >= '0' && Ch <= '9') ||
				Ch == '_' || Ch == '-';

			if (!bValid)
			{
				Ch = '_';
			}
		}

		if (Result.empty())
		{
			Result = "ImportedMesh";
		}

		return Result;
	}

	bool StartsWith(const FString& Text, const FString& Prefix)
	{
		return Text.size() >= Prefix.size() &&
			std::equal(Prefix.begin(), Prefix.end(), Text.begin());
	}

	FString TrimLeft(const FString& InText)
	{
		size_t Index = 0;
		while (Index < InText.size() && std::isspace(static_cast<unsigned char>(InText[Index])))
		{
			++Index;
		}
		return InText.substr(Index);
	}
}

//bool FAssetImportManager::ImportObjPackage(const FString& InSourceObjPath, FImportedObjPackage& OutPackage)
//{
//	OutPackage = FImportedObjPackage();
//
//	const std::filesystem::path SourceObjPath = std::filesystem::path(InSourceObjPath).lexically_normal();
//	if (!std::filesystem::exists(SourceObjPath) || SourceObjPath.extension() != ".obj")
//	{
//		return false;
//	}
//
//	// 1) obj parse 해서 mtllib 확인
//	FObjInfo ObjInfo;
//	if (!FObjParser::ParseFile(NormalizePathString(SourceObjPath), ObjInfo))
//	{
//		return false;
//	}
//
//	const std::filesystem::path SourceObjDir = SourceObjPath.parent_path();
//
//	std::filesystem::path SourceMtlPath;
//	if (!ObjInfo.MaterialLibraryPath.empty())
//	{
//		SourceMtlPath = (SourceObjDir / ObjInfo.MaterialLibraryPath).lexically_normal();
//	}
//
//	// 2) 대상 폴더 결정
//	const FString AssetName = MakeImportAssetName(NormalizePathString(SourceObjPath));
//
//	const std::filesystem::path MeshImportDir =
//		std::filesystem::path(FPaths::ToUtf8(FPaths::MeshPath()));
//
//	const std::filesystem::path TextureImportDir =
//		std::filesystem::path(FPaths::ToUtf8(FPaths::TexturePath()));
//
//	std::filesystem::create_directories(MeshImportDir);
//	std::filesystem::create_directories(TextureImportDir);
//
//	// 3) obj 복사
//	const std::filesystem::path ImportedObjPath = MeshImportDir / SourceObjPath.filename();
//	if (!CopyFileTo(SourceObjPath, ImportedObjPath))
//	{
//		return false;
//	}
//	OutPackage.ImportedObjPath = NormalizePathString(ImportedObjPath);
//
//	// 4) mtl + texture 처리
//	if (!SourceMtlPath.empty() && std::filesystem::exists(SourceMtlPath))
//	{
//		TArray<FString> TextureSourcePaths;
//		CollectTextureSourcePaths(SourceMtlPath, TextureSourcePaths);
//		// texture 복사
//		for (const FString& TextureSourcePathString : TextureSourcePaths)
//		{
//			const std::filesystem::path TextureSourcePath(TextureSourcePathString);
//			if (!std::filesystem::exists(TextureSourcePath))
//			{
//				continue;
//			}
//
//			const std::filesystem::path ImportedTexturePath = TextureImportDir / TextureSourcePath.filename();
//
//			if (CopyFileTo(TextureSourcePath, ImportedTexturePath))
//			{
//				OutPackage.ImportedTexturePaths.push_back(NormalizePathString(ImportedTexturePath));
//			}
//		}
//
//		// mtl rewrite
//		FString OriginalMtlText;
//		if (!ReadTextFile(SourceMtlPath, OriginalMtlText))
//		{
//			return false;
//		}
//
//		const std::filesystem::path ImportedMtlPath = MeshImportDir / SourceMtlPath.filename();
//		const FString RewrittenMtlText = RewriteMtlTexturePaths(
//			OriginalMtlText,
//			ImportedMtlPath,
//			TextureImportDir);
//
//		if (!WriteTextFile(ImportedMtlPath, RewrittenMtlText))
//		{
//			return false;
//		}
//
//		OutPackage.ImportedMtlPath = NormalizePathString(ImportedMtlPath);
//
//	}
//
//	return true;
//}
bool FAssetImportManager::ImportObjPackage(const FString& InSourceObjPath, FImportedObjPackage& OutPackage)
{
	OutPackage = FImportedObjPackage();

	const std::filesystem::path SourceObjPath = std::filesystem::path(InSourceObjPath).lexically_normal();
	if (!std::filesystem::exists(SourceObjPath) || SourceObjPath.extension() != ".obj")
	{
		return false;
	}

	// 1) obj parse 해서 mtllib 확인
	FObjInfo ObjInfo;
	if (!FObjParser::ParseFile(NormalizePathString(SourceObjPath), ObjInfo))
	{
		return false;
	}

	const std::filesystem::path SourceObjDir = SourceObjPath.parent_path();

	std::filesystem::path SourceMtlPath;
	if (!ObjInfo.MaterialLibraryPath.empty())
	{
		SourceMtlPath = (SourceObjDir / ObjInfo.MaterialLibraryPath).lexically_normal();
	}

	// 2) 대상 폴더 결정
	const FString AssetName = MakeImportAssetName(NormalizePathString(SourceObjPath));

	const std::filesystem::path MeshImportDir =
		std::filesystem::path(FPaths::ToUtf8(FPaths::MeshPath()));

	const std::filesystem::path TextureImportDir =
		std::filesystem::path(FPaths::ToUtf8(FPaths::TexturePath()));

	std::filesystem::create_directories(MeshImportDir);
	std::filesystem::create_directories(TextureImportDir);

	// 3) obj 복사 또는 동일 파일이면 생략
	const std::filesystem::path ImportedObjPath = (MeshImportDir / SourceObjPath.filename()).lexically_normal();
	{
		bool bNeedCopy = true;

		std::error_code ecExists;
		if (std::filesystem::exists(ImportedObjPath, ecExists))
		{
			std::error_code ecEquivalent;
			if (std::filesystem::equivalent(SourceObjPath, ImportedObjPath, ecEquivalent) && !ecEquivalent)
			{
				// 이미 Asset/Mesh 안에 있는 같은 파일을 다시 드롭한 경우
				bNeedCopy = false;
			}
		}

		if (bNeedCopy)
		{
			if (!CopyFileTo(SourceObjPath, ImportedObjPath))
			{
				return false;
			}
		}
	}
	OutPackage.ImportedObjPath = NormalizePathString(ImportedObjPath);

	// 4) mtl + texture 처리
	if (!SourceMtlPath.empty() && std::filesystem::exists(SourceMtlPath))
	{
		TArray<FString> TextureSourcePaths;
		CollectTextureSourcePaths(SourceMtlPath, TextureSourcePaths);

		// texture 복사
		for (const FString& TextureSourcePathString : TextureSourcePaths)
		{
			const std::filesystem::path TextureSourcePath = std::filesystem::path(TextureSourcePathString).lexically_normal();
			if (!std::filesystem::exists(TextureSourcePath))
			{
				continue;
			}

			const std::filesystem::path ImportedTexturePath =
				(TextureImportDir / TextureSourcePath.filename()).lexically_normal();

			bool bNeedCopyTexture = true;

			std::error_code ecExists;
			if (std::filesystem::exists(ImportedTexturePath, ecExists))
			{
				std::error_code ecEquivalent;
				if (std::filesystem::equivalent(TextureSourcePath, ImportedTexturePath, ecEquivalent) && !ecEquivalent)
				{
					bNeedCopyTexture = false;
				}
			}

			if (bNeedCopyTexture)
			{
				if (!CopyFileTo(TextureSourcePath, ImportedTexturePath))
				{
					continue;
				}
			}

			OutPackage.ImportedTexturePaths.push_back(NormalizePathString(ImportedTexturePath));
		}

		// mtl rewrite
		FString OriginalMtlText;
		if (!ReadTextFile(SourceMtlPath, OriginalMtlText))
		{
			return false;
		}

		const std::filesystem::path ImportedMtlPath =
			(MeshImportDir / SourceMtlPath.filename()).lexically_normal();

		const FString RewrittenMtlText = RewriteMtlTexturePaths(
			OriginalMtlText,
			ImportedMtlPath,
			TextureImportDir);

		// mtl은 rewrite 결과를 항상 최신 상태로 맞추기 위해 그냥 다시 씀
		if (!WriteTextFile(ImportedMtlPath, RewrittenMtlText))
		{
			return false;
		}

		OutPackage.ImportedMtlPath = NormalizePathString(ImportedMtlPath);
	}

	return true;
}

FString FAssetImportManager::MakeImportAssetName(const FString& InSourceObjPath)
{
	std::filesystem::path Path(InSourceObjPath);
	return SanitizeAssetName(Path.stem().string());
}

bool FAssetImportManager::ReadTextFile(const std::filesystem::path& InPath, FString& OutText)
{
	std::ifstream File(InPath, std::ios::in | std::ios::binary);
	if (!File.is_open())
	{
		return false;
	}

	std::ostringstream Stream;
	Stream << File.rdbuf();
	OutText = Stream.str();
	return true;
}

bool FAssetImportManager::WriteTextFile(const std::filesystem::path& InPath, const FString& InText)
{
	if (!EnsureDirectoryForFile(InPath))
	{
		return false;
	}

	std::ofstream File(InPath, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!File.is_open())
	{
		return false;
	}

	File.write(InText.data(), static_cast<std::streamsize>(InText.size()));
	return File.good();
}

bool FAssetImportManager::CopyFileTo(const std::filesystem::path& InSourcePath, const std::filesystem::path& InDestPath)
{
	if (!std::filesystem::exists(InSourcePath))
	{
		return false;
	}

	if (!EnsureDirectoryForFile(InDestPath))
	{
		return false;
	}

	std::error_code ErrorCode;
	std::filesystem::copy_file(
		InSourcePath,
		InDestPath,
		std::filesystem::copy_options::overwrite_existing,
		ErrorCode);

	return !ErrorCode;
}

bool FAssetImportManager::EnsureDirectoryForFile(const std::filesystem::path& InFilePath)
{
	std::error_code ErrorCode;
	std::filesystem::create_directories(InFilePath.parent_path(), ErrorCode);
	return !ErrorCode;
}
// #################### 무조건 여기 문제임 OutTextureSourcePaths size 0 찍힘
void FAssetImportManager::CollectTextureSourcePaths(
	const std::filesystem::path& InSourceMtlPath,
	TArray<FString>& OutTextureSourcePaths)
{
	OutTextureSourcePaths.clear();

	TMap<FString, FObjMaterialInfo> Materials;
	if (!FMtlParser::ParseFile(InSourceMtlPath.string(), Materials))
	{
		return;
	}

	const std::filesystem::path SourceMtlDir = InSourceMtlPath.parent_path();

	for (const auto& Pair : Materials)
	{
		const FObjMaterialInfo& MaterialInfo = Pair.second;
		if (MaterialInfo.DiffuseTexturePath.empty())
		{
			continue;
		}

		std::filesystem::path TexturePath(MaterialInfo.DiffuseTexturePath);
		if (!TexturePath.is_absolute())
		{
			TexturePath = SourceMtlDir / TexturePath;
		}

		TexturePath = TexturePath.lexically_normal();

		const FString Normalized = NormalizePathString(TexturePath);
		if (std::find(OutTextureSourcePaths.begin(), OutTextureSourcePaths.end(), Normalized) == OutTextureSourcePaths.end())
		{
			OutTextureSourcePaths.push_back(Normalized);
		}
	}
}

FString FAssetImportManager::RewriteMtlTexturePaths(
	const FString& InOriginalMtlText,
	const std::filesystem::path& InImportedMtlPath,
	const std::filesystem::path& InImportedTextureDir)
{
	std::stringstream Input(InOriginalMtlText);
	std::stringstream Output;

	const std::filesystem::path ImportedMtlDir = InImportedMtlPath.parent_path();

	FString Line;
	while (std::getline(Input, Line))
	{
		FString RawLine = Line;
		if (!RawLine.empty() && RawLine.back() == '\r')
		{
			RawLine.pop_back();
		}

		const FString Trimmed = TrimLeft(RawLine);

		if (StartsWith(Trimmed, "map_Kd "))
		{
			// map_Kd 뒤에 오는 원본 경로는 버리고 filename 기준으로 새 경로 작성
			const FString OriginalTextureRef = Trimmed.substr(7);
			std::filesystem::path OriginalTexturePath(OriginalTextureRef);

			const std::filesystem::path ImportedTexturePath =
				(InImportedTextureDir / OriginalTexturePath.filename()).lexically_normal();

			std::filesystem::path RelativePath =
				std::filesystem::relative(ImportedTexturePath, ImportedMtlDir);

			Output << "map_Kd " << RelativePath.generic_string() << "\n";
		}
		else
		{
			Output << RawLine << "\n";
		}
	}

	return Output.str();
}

bool FAssetImportManager::ImportStaticMeshBin(const FString& InSourceBinPath, FString& OutImportedBinPath)
{
	OutImportedBinPath.clear();

	const std::filesystem::path SourcePath =
		std::filesystem::path(InSourceBinPath).lexically_normal();

	if (!std::filesystem::exists(SourcePath))
	{
		return false;
	}

	std::string Ext = SourcePath.extension().string();
	std::transform(Ext.begin(), Ext.end(), Ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (Ext != ".smbin")
	{
		return false;
	}

	const std::filesystem::path MeshImportDir =
		std::filesystem::path(FPaths::ToUtf8(FPaths::MeshPath()));

	std::filesystem::create_directories(MeshImportDir);

	const std::filesystem::path ImportedPath = MeshImportDir / SourcePath.filename();

	// 이미 Asset/Mesh 안에 있는 같은 파일이면 그냥 성공 처리
	{
		std::error_code ec;
		if (std::filesystem::exists(ImportedPath, ec))
		{
			std::error_code ec2;
			if (std::filesystem::equivalent(SourcePath, ImportedPath, ec2) && !ec2)
			{
				OutImportedBinPath = NormalizePathString(ImportedPath);
				return true;
			}

			// 같은 파일은 아니지만 동일한 이름의 파일이 이미 있으면 제거 후 복사
			std::error_code ec3;
			std::filesystem::remove(ImportedPath, ec3);
			if (ec3)
			{
				return false;
			}
		}
	}

	if (!CopyFileTo(SourcePath, ImportedPath))
	{
		return false;
	}

	OutImportedBinPath = NormalizePathString(ImportedPath);
	return true;
}
