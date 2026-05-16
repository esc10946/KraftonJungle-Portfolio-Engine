#include "Core/AnimationClipLoadService.h"

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
}

FAnimationClipLoadService::FAnimationClipLoadService(FResourceManager& InResourceManager)
    : ResourceManager(InResourceManager)
{
}

UAnimationSequence* FAnimationClipLoadService::Load(const FString& Path)
{
    const FString NormalizedPath = FPaths::Normalize(Path);

    if (UAnimationSequence* FoundClip = ResourceManager.FindAnimationClip(NormalizedPath))
    {
        return FoundClip;
    }

    if (IsBinaryPath(NormalizedPath))
    {
        return LoadBinary(NormalizedPath, NormalizedPath);
    }

    if (UAnimationSequence* ImportedClip = LoadSiblingImportedBinary(NormalizedPath))
    {
        ResourceManager.AnimationClipMap[NormalizedPath] = ImportedClip;
        return ImportedClip;
    }

    return LoadSourceOrCachedBinary(NormalizedPath);
}

UAnimationSequence* FAnimationClipLoadService::LoadBinary(const FString& BinaryPath, const FString& CacheKey)
{
    FAnimationSequence* LoadedClipData = new FAnimationSequence();
    if (!ResourceManager.AnimationClipSerializer.LoadAnimationClip(BinaryPath, *LoadedClipData))
    {
        delete LoadedClipData;
        return nullptr;
    }

    UE_LOG("[AnimationClipLoad] Source=ImportedBinary | Path=%s", BinaryPath.c_str());
    return FinalizeLoadedClip(LoadedClipData, CacheKey);
}

UAnimationSequence* FAnimationClipLoadService::LoadSiblingImportedBinary(const FString& NormalizedPath)
{
    const TArray<FString> BinaryPaths = FindSiblingAnimationBinaries(NormalizedPath);
    for (const FString& BinaryPath : BinaryPaths)
    {
        if (UAnimationSequence* Clip = LoadBinary(BinaryPath, BinaryPath))
        {
            return Clip;
        }
    }

    return nullptr;
}

UAnimationSequence* FAnimationClipLoadService::LoadSourceOrCachedBinary(const FString& NormalizedPath)
{
    const FString BinaryPath = FAssetPathPolicy::MakeWritableAnimationClipCacheBinaryPath(NormalizedPath);

    FAnimationSequence* LoadedClipData = nullptr;
    double BinaryLoadSec = 0.0;
    double SourceLoadSec = 0.0;

    if (ResourceManager.IsAnimationClipBinaryValid(NormalizedPath, BinaryPath))
    {
        const auto BinaryStart = std::chrono::steady_clock::now();

        LoadedClipData = new FAnimationSequence();
        if (!ResourceManager.AnimationClipSerializer.LoadAnimationClip(BinaryPath, *LoadedClipData))
        {
            delete LoadedClipData;
            LoadedClipData = nullptr;
        }

        const auto BinaryEnd = std::chrono::steady_clock::now();
        BinaryLoadSec = std::chrono::duration<double>(BinaryEnd - BinaryStart).count();
    }

    if (LoadedClipData == nullptr)
    {
        const auto SourceStart = std::chrono::steady_clock::now();
        const TArray<USkeletonAsset*> ImportedSkeletons = ResourceManager.ImportSkeletonsFromFbx(NormalizedPath);
        const USkeletonAsset* TargetSkeleton = ImportedSkeletons.empty() ? nullptr : ImportedSkeletons[0];

        FAnimationImportOptions ImportOptions;
        ImportOptions.bImportAllStacks = true;
        TArray<FAnimationClip*> ImportedClips = ResourceManager.FbxImporter.LoadAnimations(NormalizedPath, ImportOptions);
        const auto SourceEnd = std::chrono::steady_clock::now();
        SourceLoadSec = std::chrono::duration<double>(SourceEnd - SourceStart).count();

        if (ImportedClips.empty() || ImportedClips[0] == nullptr)
        {
            UE_LOG_ERROR("[AnimationClipLoad] Failed | Path=%s | BinarySec=%.6f | FbxSec=%.6f",
                         NormalizedPath.c_str(), BinaryLoadSec, SourceLoadSec);
            for (FAnimationClip* Clip : ImportedClips)
            {
                delete Clip;
            }
            return nullptr;
        }

        UAnimationSequence* FirstLoadedClip = nullptr;
        for (FAnimationClip* Clip : ImportedClips)
        {
            if (!Clip)
            {
                continue;
            }

            const FString ClipToken = Clip->Name.empty() ? FString("anim") : Clip->Name;
            const FString SaveBinaryPath = FAssetPathPolicy::MakeSiblingAnimationClipBinaryPath(NormalizedPath, ClipToken);

            if (TargetSkeleton)
            {
                Clip->SkeletonSourcePath = FAssetPathPolicy::MakeAssetRelativePath(
                    SaveBinaryPath,
                    TargetSkeleton->GetAssetPathFileName());
            }

            const bool bSaveBinaryOk = ResourceManager.AnimationClipSerializer.SaveAnimationClip(SaveBinaryPath, NormalizedPath, *Clip);
            if (bSaveBinaryOk)
            {
                UE_LOG("[AnimationClipLoad] Source=FBX | Path=%s | FbxSec=%.6f | BinarySave=OK | BinaryPath=%s",
                       NormalizedPath.c_str(), SourceLoadSec, SaveBinaryPath.c_str());

                UAnimationSequence* LoadedClip = FinalizeLoadedClip(Clip, SaveBinaryPath);
                if (!FirstLoadedClip)
                {
                    FirstLoadedClip = LoadedClip;
                }
                continue;
            }

            UE_LOG_WARNING("[AnimationClipLoad] Source=FBX | Path=%s | FbxSec=%.6f | BinarySave=FAIL | BinaryPath=%s",
                           NormalizedPath.c_str(), SourceLoadSec, SaveBinaryPath.c_str());
            delete Clip;
        }

        if (!FirstLoadedClip)
        {
            return nullptr;
        }

        ResourceManager.AnimationClipMap[NormalizedPath] = FirstLoadedClip;
        return FirstLoadedClip;
    }
    else
    {
        UE_LOG("[AnimationClipLoad] Source=Binary | Path=%s | BinarySec=%.6f | BinaryPath=%s",
               NormalizedPath.c_str(), BinaryLoadSec, BinaryPath.c_str());
    }

    return FinalizeLoadedClip(LoadedClipData, NormalizedPath);
}

UAnimationSequence* FAnimationClipLoadService::FinalizeLoadedClip(FAnimationSequence* SequenceData, const FString& CacheKey)
{
    UAnimationSequence* LoadedClip = UObjectManager::Get().CreateObject<UAnimationSequence>();
    LoadedClip->SetSequenceData(SequenceData);

    ResourceManager.AnimationClipMap[CacheKey] = LoadedClip;
    if (std::find(ResourceManager.AnimationClipFilePaths.begin(), ResourceManager.AnimationClipFilePaths.end(), CacheKey)
        == ResourceManager.AnimationClipFilePaths.end())
    {
        ResourceManager.AnimationClipFilePaths.push_back(CacheKey);
    }

    const FAnimationSequence* Clip = LoadedClip->GetSequenceData();
    UE_LOG("[AnimationClipLoad] Loaded | Path=%s | Name=%s | BoneTracks=%zu | ShapeKeyTracks=%zu | Duration=%.3f",
           CacheKey.c_str(),
           Clip ? Clip->Name.c_str() : "",
           Clip ? Clip->BoneTracks.size() : 0,
           Clip ? Clip->ShapeKeyTracks.size() : 0,
           Clip ? Clip->DurationSeconds : 0.0f);

    return LoadedClip;
}
