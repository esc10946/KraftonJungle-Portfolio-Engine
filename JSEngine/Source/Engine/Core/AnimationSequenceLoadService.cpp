#include "Core/AnimationSequenceLoadService.h"

#include "Asset/AnimationSequence.h"
#include "Core/AssetPathPolicy.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"

#include <algorithm>
#include <chrono>
#include <cwctype>
#include <filesystem>

namespace
{
    bool IsBinaryPath(const FString& Path)
    {
        std::filesystem::path FsPath(FPaths::ToWide(FPaths::Normalize(Path)));
        std::wstring Extension = FsPath.extension().wstring();
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::towlower);
        return Extension == L".bin";
    }

    bool IsFbxPath(const FString& Path)
    {
        std::filesystem::path FsPath(FPaths::ToWide(FPaths::Normalize(Path)));
        std::wstring Extension = FsPath.extension().wstring();
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::towlower);
        return Extension == L".fbx";
    }

    TArray<FString> FindSiblingAnimationBinaries(const FString& SourceFbxPath)
    {
        TArray<FString> Result;

        const FString NormalizedSourcePath = FPaths::Normalize(SourceFbxPath);
        const std::filesystem::path SourceFsPath(FPaths::ToWide(NormalizedSourcePath));
        const std::filesystem::path ParentPath = SourceFsPath.parent_path();

        std::error_code Ec;
        if (ParentPath.empty() || !std::filesystem::exists(ParentPath, Ec) || Ec)
        {
            return Result;
        }

        const FString Prefix = FPaths::ToUtf8(SourceFsPath.stem().generic_wstring()) + "_anim_";
        for (const std::filesystem::directory_entry& Entry : std::filesystem::directory_iterator(ParentPath, Ec))
        {
            if (Ec)
            {
                break;
            }

            if (!Entry.is_regular_file(Ec) || Ec)
            {
                continue;
            }

            std::wstring Extension = Entry.path().extension().wstring();
            std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::towlower);
            if (Extension != L".bin")
            {
                continue;
            }

            const FString FileName = FPaths::ToUtf8(Entry.path().filename().generic_wstring());
            if (FileName.rfind(Prefix, 0) == 0)
            {
                Result.push_back(FPaths::Normalize(FPaths::ToUtf8(Entry.path().generic_wstring())));
            }
        }

        std::sort(Result.begin(), Result.end());
        return Result;
    }

    TArray<FString> FindSiblingAnimationAssets(const FString& SourceFbxPath)
    {
        TArray<FString> Result;

        const FString NormalizedSourcePath = FPaths::Normalize(SourceFbxPath);
        const std::filesystem::path SourceFsPath(FPaths::ToWide(NormalizedSourcePath));
        const std::filesystem::path ParentPath = SourceFsPath.parent_path();

        std::error_code Ec;
        if (ParentPath.empty() || !std::filesystem::exists(ParentPath, Ec) || Ec)
        {
            return Result;
        }

        const FString Prefix = FPaths::ToUtf8(SourceFsPath.stem().generic_wstring()) + "_anim_";
        for (const std::filesystem::directory_entry& Entry : std::filesystem::directory_iterator(ParentPath, Ec))
        {
            if (Ec)
            {
                break;
            }

            if (!Entry.is_regular_file(Ec) || Ec)
            {
                continue;
            }

            std::wstring Extension = Entry.path().extension().wstring();
            std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::towlower);
            if (Extension != L".anim")
            {
                continue;
            }

            const FString FileName = FPaths::ToUtf8(Entry.path().filename().generic_wstring());
            if (FileName.rfind(Prefix, 0) == 0)
            {
                Result.push_back(FPaths::Normalize(FPaths::ToUtf8(Entry.path().generic_wstring())));
            }
        }

        std::sort(Result.begin(), Result.end());
        return Result;
    }
}

FAnimationSequenceLoadService::FAnimationSequenceLoadService(FResourceManager& InResourceManager)
    : ResourceManager(InResourceManager)
{
}

UAnimationSequence* FAnimationSequenceLoadService::Load(const FString& Path)
{
    const FString NormalizedPath = FPaths::Normalize(Path);

    if (UAnimationSequence* FoundSequence = ResourceManager.FindAnimationSequence(NormalizedPath))
    {
        return FoundSequence;
    }

    if (FAssetPathPolicy::IsAnimationSequenceAssetPath(NormalizedPath))
    {
        return LoadAnimationAsset(NormalizedPath);
    }

    if (UAnimationSequence* ImportedAssetSequence = LoadSiblingAnimationAsset(NormalizedPath))
    {
        ResourceManager.AnimationSequenceMap[NormalizedPath] = ImportedAssetSequence;
        return ImportedAssetSequence;
    }

    if (IsBinaryPath(NormalizedPath))
    {
        return LoadBinary(NormalizedPath, NormalizedPath);
    }

    if (UAnimationSequence* ImportedSequence = LoadSiblingImportedBinary(NormalizedPath))
    {
        ResourceManager.AnimationSequenceMap[NormalizedPath] = ImportedSequence;
        return ImportedSequence;
    }

    if (IsFbxPath(NormalizedPath))
    {
        UE_LOG_WARNING("[AnimationSequenceLoad] Imported FBX animation binary missing. Explicit import is required. | Path=%s", NormalizedPath.c_str());
    }
    else
    {
        UE_LOG_WARNING("[AnimationSequenceLoad] Unsupported animation source path | Path=%s", NormalizedPath.c_str());
    }
    return nullptr;
}

UAnimationSequence* FAnimationSequenceLoadService::ImportFbxSource(const FString& Path)
{
    const FString NormalizedPath = FPaths::Normalize(Path);
    if (!IsFbxPath(NormalizedPath))
    {
        UE_LOG_WARNING("[AnimationSequenceLoad] ImportFbxSource only supports FBX | Path=%s", NormalizedPath.c_str());
        return nullptr;
    }

    return ImportFbxSourceToBinary(NormalizedPath);
}

UAnimationSequence* FAnimationSequenceLoadService::LoadAnimationAsset(const FString& AssetPath)
{
    UAnimationSequence* LoadedSequence = UObjectManager::Get().CreateObject<UAnimationSequence>();
    if (!ResourceManager.AnimationSequenceSerializer.LoadAnimationSequenceAsset(AssetPath, *LoadedSequence))
    {
        UObjectManager::Get().DestroyObject(LoadedSequence);
        return nullptr;
    }

    ResourceManager.AnimationSequenceMap[AssetPath] = LoadedSequence;
    if (std::find(ResourceManager.AnimationSequenceFilePaths.begin(), ResourceManager.AnimationSequenceFilePaths.end(), AssetPath)
        == ResourceManager.AnimationSequenceFilePaths.end())
    {
        ResourceManager.AnimationSequenceFilePaths.push_back(AssetPath);
    }

    const FAnimationSequence* Sequence = LoadedSequence->GetSequenceData();
    UE_LOG("[AnimationSequenceLoad] Source=AnimAsset | Path=%s | Name=%s | Notifies=%zu | Duration=%.3f",
           AssetPath.c_str(),
           Sequence ? Sequence->Name.c_str() : "",
           LoadedSequence->GetNotifies().size(),
           Sequence ? Sequence->DurationSeconds : 0.0f);
	
	LoadedSequence->ResetDirty();
    return LoadedSequence;
}

UAnimationSequence* FAnimationSequenceLoadService::LoadSiblingAnimationAsset(const FString& NormalizedPath)
{
    const TArray<FString> AssetPaths = FindSiblingAnimationAssets(NormalizedPath);
    for (const FString& AssetPath : AssetPaths)
    {
        if (UAnimationSequence* Sequence = LoadAnimationAsset(AssetPath))
        {
            return Sequence;
        }
    }

    return nullptr;
}

UAnimationSequence* FAnimationSequenceLoadService::LoadBinary(const FString& BinaryPath, const FString& CacheKey)
{
    FAnimationSequence* LoadedSequenceData = new FAnimationSequence();
    if (!ResourceManager.AnimationSequenceSerializer.LoadAnimationSequence(BinaryPath, *LoadedSequenceData))
    {
        delete LoadedSequenceData;
        return nullptr;
    }

    UE_LOG("[AnimationSequenceLoad] Source=ImportedBinary | Path=%s", BinaryPath.c_str());
    return FinalizeLoadedSequence(LoadedSequenceData, CacheKey);
}

UAnimationSequence* FAnimationSequenceLoadService::LoadSiblingImportedBinary(const FString& NormalizedPath)
{
    const TArray<FString> BinaryPaths = FindSiblingAnimationBinaries(NormalizedPath);
    for (const FString& BinaryPath : BinaryPaths)
    {
        if (UAnimationSequence* Sequence = LoadBinary(BinaryPath, BinaryPath))
        {
            return Sequence;
        }
    }

    return nullptr;
}

UAnimationSequence* FAnimationSequenceLoadService::ImportFbxSourceToBinary(const FString& NormalizedPath)
{
    const auto SourceStart = std::chrono::steady_clock::now();
    const TArray<USkeletonAsset*> ImportedSkeletons = ResourceManager.ImportSkeletonsFromFbx(NormalizedPath);
    const USkeletonAsset* TargetSkeleton = ImportedSkeletons.empty() ? nullptr : ImportedSkeletons[0];

    FAnimationImportOptions ImportOptions;
    ImportOptions.bImportAllStacks = true;
    TArray<FAnimationSequence*> ImportedSequences = ResourceManager.FbxImporter.LoadAnimationSequences(NormalizedPath, ImportOptions);
    const auto SourceEnd = std::chrono::steady_clock::now();
    const double SourceLoadSec = std::chrono::duration<double>(SourceEnd - SourceStart).count();

    if (ImportedSequences.empty() || ImportedSequences[0] == nullptr)
    {
        UE_LOG_ERROR("[AnimationSequenceLoad] Failed import | Path=%s | FbxSec=%.6f",
                     NormalizedPath.c_str(), SourceLoadSec);
        for (FAnimationSequence* Sequence : ImportedSequences)
        {
            delete Sequence;
        }
        return nullptr;
    }

    UAnimationSequence* FirstLoadedSequence = nullptr;
    for (FAnimationSequence* Sequence : ImportedSequences)
    {
        if (!Sequence)
        {
            continue;
        }

        const FString SequenceToken = Sequence->Name.empty() ? FString("anim") : Sequence->Name;
        const FString SaveBinaryPath = FAssetPathPolicy::MakeSiblingAnimationSequenceBinaryPath(NormalizedPath, SequenceToken);
        const FString SaveAssetPath = FAssetPathPolicy::MakeSiblingAnimationSequenceAssetPath(NormalizedPath, SequenceToken);

        if (TargetSkeleton)
        {
            Sequence->SkeletonSourcePath = FAssetPathPolicy::MakeAssetRelativePath(
                SaveAssetPath,
                TargetSkeleton->GetAssetPathFileName());
        }

        const bool bSaveBinaryOk = ResourceManager.AnimationSequenceSerializer.SaveAnimationSequence(SaveBinaryPath, NormalizedPath, *Sequence);
        if (bSaveBinaryOk)
        {
            UAnimationSequence* LoadedSequence = FinalizeLoadedSequence(Sequence, SaveAssetPath);
            LoadedSequence->SetAssetPath(SaveAssetPath);
            LoadedSequence->SetSourceImportPath(NormalizedPath);

            const bool bSaveAssetOk = ResourceManager.AnimationSequenceSerializer.SaveAnimationSequenceAsset(SaveAssetPath, *LoadedSequence);
            UE_LOG("[AnimationSequenceLoad] Source=FBX | Path=%s | FbxSec=%.6f | BinarySave=OK | BinaryPath=%s | AnimSave=%s | AnimPath=%s",
                   NormalizedPath.c_str(),
                   SourceLoadSec,
                   SaveBinaryPath.c_str(),
                   bSaveAssetOk ? "OK" : "FAIL",
                   SaveAssetPath.c_str());

            if (!FirstLoadedSequence)
            {
                FirstLoadedSequence = LoadedSequence;
            }
            continue;
        }

        UE_LOG_WARNING("[AnimationSequenceLoad] Source=FBX | Path=%s | FbxSec=%.6f | BinarySave=FAIL | BinaryPath=%s",
                       NormalizedPath.c_str(), SourceLoadSec, SaveBinaryPath.c_str());
        delete Sequence;
    }

    if (!FirstLoadedSequence)
    {
        return nullptr;
    }

    ResourceManager.AnimationSequenceMap[NormalizedPath] = FirstLoadedSequence;
    return FirstLoadedSequence;
}

UAnimationSequence* FAnimationSequenceLoadService::FinalizeLoadedSequence(FAnimationSequence* SequenceData, const FString& CacheKey)
{
    UAnimationSequence* LoadedSequence = UObjectManager::Get().CreateObject<UAnimationSequence>();
    LoadedSequence->SetSequenceData(SequenceData);

    ResourceManager.AnimationSequenceMap[CacheKey] = LoadedSequence;
    if (std::find(ResourceManager.AnimationSequenceFilePaths.begin(), ResourceManager.AnimationSequenceFilePaths.end(), CacheKey)
        == ResourceManager.AnimationSequenceFilePaths.end())
    {
        ResourceManager.AnimationSequenceFilePaths.push_back(CacheKey);
    }

    const FAnimationSequence* Sequence = LoadedSequence->GetSequenceData();
    UE_LOG("[AnimationSequenceLoad] Loaded | Path=%s | Name=%s | BoneTracks=%zu | ShapeKeyTracks=%zu | Duration=%.3f",
           CacheKey.c_str(),
           Sequence ? Sequence->Name.c_str() : "",
           Sequence ? Sequence->BoneTracks.size() : 0,
           Sequence ? Sequence->ShapeKeyTracks.size() : 0,
           Sequence ? Sequence->DurationSeconds : 0.0f);

    return LoadedSequence;
}
