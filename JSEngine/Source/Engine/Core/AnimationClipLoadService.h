#pragma once

#include "Core/CoreMinimal.h"

class FResourceManager;
class UAnimationClipAsset;
struct FAnimationClip;

class FAnimationClipLoadService
{
public:
    explicit FAnimationClipLoadService(FResourceManager& InResourceManager);

    UAnimationClipAsset* Load(const FString& Path);

private:
    UAnimationClipAsset* LoadBinary(const FString& BinaryPath, const FString& CacheKey);
    UAnimationClipAsset* LoadSiblingImportedBinary(const FString& NormalizedPath);
    UAnimationClipAsset* LoadSourceOrCachedBinary(const FString& NormalizedPath);
    UAnimationClipAsset* FinalizeLoadedClip(FAnimationClip* ClipData, const FString& CacheKey);

    FResourceManager& ResourceManager;
};

