#pragma once

#include "Asset/AnimationTypes.h"
#include "Core/CoreMinimal.h"

class FResourceManager;
class UAnimationSequence;

class FAnimationClipLoadService
{
public:
    explicit FAnimationClipLoadService(FResourceManager& InResourceManager);

    UAnimationSequence* Load(const FString& Path);

private:
    UAnimationSequence* LoadBinary(const FString& BinaryPath, const FString& CacheKey);
    UAnimationSequence* LoadSiblingImportedBinary(const FString& NormalizedPath);
    UAnimationSequence* LoadSourceOrCachedBinary(const FString& NormalizedPath);
    UAnimationSequence* FinalizeLoadedClip(FAnimationSequence* SequenceData, const FString& CacheKey);

    FResourceManager& ResourceManager;
};

