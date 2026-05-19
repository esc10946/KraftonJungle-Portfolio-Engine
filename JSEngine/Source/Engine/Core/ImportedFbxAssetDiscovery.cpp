#include "Core/ImportedFbxAssetDiscovery.h"

#include "Asset/AnimationSequenceSerializer.h"
#include "Asset/BinarySerializer.h"
#include "Asset/SkeletalMeshTypes.h"
#include "Asset/StaticMeshTypes.h"
#include "Asset/SkeletonSerializer.h"
#include "Core/AssetPathPolicy.h"
#include "Core/Paths.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>

namespace
{
    bool IsBinPath(const std::filesystem::path& Path)
    {
        std::wstring Extension = Path.extension().wstring();
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::towlower);
        return Extension == L".bin";
    }

    FString NormalizePath(const std::filesystem::path& Path)
    {
        return FPaths::Normalize(FPaths::ToUtf8(Path.generic_wstring()));
    }

	bool IsSiblingImportedAssetForSource(const std::filesystem::path& BinaryPath, const std::filesystem::path& SourceFbxPath)
	{
		if (BinaryPath.parent_path() != SourceFbxPath.parent_path())
		{
            return false;
        }

        const FString Stem = FPaths::ToUtf8(SourceFbxPath.stem().generic_wstring());
        const FString FileName = FPaths::ToUtf8(BinaryPath.filename().generic_wstring());
		return FileName.rfind(Stem + "_skeleton_", 0) == 0
			|| FileName.rfind(Stem + "_skeletalmesh_", 0) == 0
			|| FileName.rfind(Stem + "_anim_", 0) == 0;
	}

    void AddRecordIfUnique(TArray<FImportedFbxAssetRecord>& Records, const FImportedFbxAssetRecord& Record)
    {
        if (Record.AssetPath.empty())
        {
            return;
        }

        auto ExistingIt = std::find_if(
            Records.begin(),
            Records.end(),
            [&Record](const FImportedFbxAssetRecord& Existing)
            {
                return Existing.AssetPath == Record.AssetPath;
            });

        if (ExistingIt == Records.end())
        {
            Records.push_back(Record);
        }
    }
}

bool FImportedFbxAssetDiscovery::ReadImportedAssetRecord(const FString& BinaryPath, FImportedFbxAssetRecord& OutRecord) const
{
    const FString NormalizedBinaryPath = FPaths::Normalize(BinaryPath);

    {
        FSkeletonSerializer Serializer;
        FSkeleton Skeleton;
        if (Serializer.LoadSkeleton(NormalizedBinaryPath, Skeleton))
        {
            OutRecord = {};
            OutRecord.Type = EImportedFbxAssetType::Skeleton;
            OutRecord.AssetPath = NormalizedBinaryPath;
            OutRecord.Name = Skeleton.RootNodeName;
            OutRecord.BoneCount = static_cast<uint32>(Skeleton.Bones.size());
            return true;
        }
    }

    {
        FBinarySerializer Serializer;
        FStaticMesh Mesh;
        if (Serializer.LoadStaticMesh(NormalizedBinaryPath, Mesh))
        {
            OutRecord = {};
            OutRecord.Type = EImportedFbxAssetType::StaticMesh;
            OutRecord.AssetPath = NormalizedBinaryPath;
            OutRecord.SourcePath = Mesh.PathFileName;
            OutRecord.Name = FPaths::ToUtf8(std::filesystem::path(FPaths::ToWide(NormalizedBinaryPath)).stem().wstring());
            return true;
        }
    }

    {
        FBinarySerializer Serializer;
        FSkeletalMesh Mesh;
        if (Serializer.LoadSkeletalMesh(NormalizedBinaryPath, Mesh))
        {
            OutRecord = {};
            OutRecord.Type = EImportedFbxAssetType::SkeletalMesh;
            OutRecord.AssetPath = NormalizedBinaryPath;
            OutRecord.SourcePath = Mesh.PathFileName;
            OutRecord.SkeletonSourcePath = Mesh.SkeletonSourcePath;
            OutRecord.BoneCount = static_cast<uint32>(Mesh.Bones.size());
            return true;
        }
    }

    {
        FAnimationSequenceSerializer Serializer;
        FAnimationSequence Sequence;
        if (Serializer.LoadAnimationSequence(NormalizedBinaryPath, Sequence))
        {
            OutRecord = {};
            OutRecord.Type = EImportedFbxAssetType::AnimationSequence;
            OutRecord.AssetPath = NormalizedBinaryPath;
            OutRecord.SourcePath = Sequence.SourcePath;
            OutRecord.Name = Sequence.Name;
            OutRecord.SkeletonSourcePath = Sequence.SkeletonSourcePath;
            return true;
        }
    }

    return false;
}

TArray<FImportedFbxAssetRecord> FImportedFbxAssetDiscovery::DiscoverInDirectory(const FString& DirectoryPath) const
{
    TArray<FImportedFbxAssetRecord> Result;

    const std::filesystem::path RootPath(FPaths::ToWide(FPaths::Normalize(DirectoryPath)));
    std::error_code Ec;
    if (!std::filesystem::exists(RootPath, Ec) || !std::filesystem::is_directory(RootPath, Ec) || Ec)
    {
        return Result;
    }

    for (const std::filesystem::directory_entry& Entry : std::filesystem::recursive_directory_iterator(RootPath, std::filesystem::directory_options::skip_permission_denied, Ec))
    {
        if (Ec)
        {
            break;
        }

        if (!Entry.is_regular_file(Ec) || Ec || !IsBinPath(Entry.path()))
        {
            continue;
        }

        FImportedFbxAssetRecord Record;
        if (ReadImportedAssetRecord(NormalizePath(Entry.path()), Record))
        {
            Result.push_back(Record);
        }
    }

    std::sort(
        Result.begin(),
        Result.end(),
        [](const FImportedFbxAssetRecord& A, const FImportedFbxAssetRecord& B)
        {
            return A.AssetPath < B.AssetPath;
        });

    return Result;
}

TArray<FImportedFbxAssetRecord> FImportedFbxAssetDiscovery::DiscoverForSourceFbx(const FString& SourceFbxPath) const
{
    TArray<FImportedFbxAssetRecord> Result;

    const FString NormalizedSourceFbxPath = FPaths::Normalize(SourceFbxPath);
    const std::filesystem::path SourcePath(FPaths::ToWide(NormalizedSourceFbxPath));
    const std::filesystem::path ParentPath = SourcePath.parent_path();

    std::error_code Ec;
    if (ParentPath.empty() || !std::filesystem::exists(ParentPath, Ec) || Ec)
    {
        return Result;
    }

    for (const std::filesystem::directory_entry& Entry : std::filesystem::directory_iterator(ParentPath, Ec))
    {
        if (Ec)
        {
            break;
        }

        if (!Entry.is_regular_file(Ec) || Ec || !IsBinPath(Entry.path()))
        {
            continue;
        }

        if (!IsSiblingImportedAssetForSource(Entry.path(), SourcePath))
        {
            continue;
        }

        FImportedFbxAssetRecord Record;
        if (ReadImportedAssetRecord(NormalizePath(Entry.path()), Record))
        {
            AddRecordIfUnique(Result, Record);
        }
    }

    const FString StaticMeshBinaryPath = FAssetPathPolicy::MakeStaticMeshCacheBinaryPath(NormalizedSourceFbxPath);
    FImportedFbxAssetRecord StaticMeshRecord;
    if (ReadImportedAssetRecord(StaticMeshBinaryPath, StaticMeshRecord) &&
        StaticMeshRecord.Type == EImportedFbxAssetType::StaticMesh &&
        FPaths::Normalize(StaticMeshRecord.SourcePath) == NormalizedSourceFbxPath)
    {
        AddRecordIfUnique(Result, StaticMeshRecord);
    }

    std::sort(
        Result.begin(),
        Result.end(),
        [](const FImportedFbxAssetRecord& A, const FImportedFbxAssetRecord& B)
        {
            return A.AssetPath < B.AssetPath;
        });

    return Result;
}
