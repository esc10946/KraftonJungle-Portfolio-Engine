#include "Core/AnimationClipLoadService.h"

#include "Asset/AnimationClipAsset.h"
#include "Core/AssetPathPolicy.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"

#include <algorithm>
#include <chrono>

FAnimationClipLoadService::FAnimationClipLoadService(FResourceManager& InResourceManager)
    : ResourceManager(InResourceManager)
{
}

UAnimationClipAsset* FAnimationClipLoadService::Load(const FString& Path)
{
    const FString NormalizedPath = FPaths::Normalize(Path);

    if (UAnimationClipAsset* FoundClip = ResourceManager.FindAnimationClip(NormalizedPath))
    {
        return FoundClip;
    }

    return LoadSourceOrCachedBinary(NormalizedPath);
}

UAnimationClipAsset* FAnimationClipLoadService::LoadSourceOrCachedBinary(const FString& NormalizedPath)
{
    const FString BinaryPath = FAssetPathPolicy::MakeWritableAnimationClipCacheBinaryPath(NormalizedPath);

    FAnimationClip* LoadedClipData = nullptr;
    double BinaryLoadSec = 0.0;
    double SourceLoadSec = 0.0;

    if (ResourceManager.IsAnimationClipBinaryValid(NormalizedPath, BinaryPath))
    {
        const auto BinaryStart = std::chrono::steady_clock::now();

        LoadedClipData = new FAnimationClip();
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
        FAnimationImportOptions ImportOptions;
        ImportOptions.bImportAllStacks = false;
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

        LoadedClipData = ImportedClips[0];
        for (size_t Index = 1; Index < ImportedClips.size(); ++Index)
        {
            delete ImportedClips[Index];
        }

        const bool bSaveBinaryOk = ResourceManager.AnimationClipSerializer.SaveAnimationClip(BinaryPath, NormalizedPath, *LoadedClipData);
        if (bSaveBinaryOk)
        {
            UE_LOG("[AnimationClipLoad] Source=FBX | Path=%s | FbxSec=%.6f | BinarySave=OK | BinaryPath=%s",
                   NormalizedPath.c_str(), SourceLoadSec, BinaryPath.c_str());
        }
        else
        {
            UE_LOG_WARNING("[AnimationClipLoad] Source=FBX | Path=%s | FbxSec=%.6f | BinarySave=FAIL | BinaryPath=%s",
                           NormalizedPath.c_str(), SourceLoadSec, BinaryPath.c_str());
        }
    }
    else
    {
        UE_LOG("[AnimationClipLoad] Source=Binary | Path=%s | BinarySec=%.6f | BinaryPath=%s",
               NormalizedPath.c_str(), BinaryLoadSec, BinaryPath.c_str());
    }

    return FinalizeLoadedClip(LoadedClipData, NormalizedPath);
}

UAnimationClipAsset* FAnimationClipLoadService::FinalizeLoadedClip(FAnimationClip* ClipData, const FString& CacheKey)
{
    UAnimationClipAsset* LoadedClip = UObjectManager::Get().CreateObject<UAnimationClipAsset>();
    LoadedClip->SetClipData(ClipData);

    ResourceManager.AnimationClipMap[CacheKey] = LoadedClip;
    if (std::find(ResourceManager.AnimationClipFilePaths.begin(), ResourceManager.AnimationClipFilePaths.end(), CacheKey)
        == ResourceManager.AnimationClipFilePaths.end())
    {
        ResourceManager.AnimationClipFilePaths.push_back(CacheKey);
    }

    const FAnimationClip* Clip = LoadedClip->GetClipData();
    UE_LOG("[AnimationClipLoad] Loaded | Path=%s | Name=%s | BoneTracks=%zu | ShapeKeyTracks=%zu | Duration=%.3f",
           CacheKey.c_str(),
           Clip ? Clip->Name.c_str() : "",
           Clip ? Clip->BoneTracks.size() : 0,
           Clip ? Clip->ShapeKeyTracks.size() : 0,
           Clip ? Clip->DurationSeconds : 0.0f);

    return LoadedClip;
}
