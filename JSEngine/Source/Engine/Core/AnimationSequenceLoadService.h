#pragma once

#include "Asset/AnimationTypes.h"
#include "Core/CoreMinimal.h"

class FResourceManager;
class UAnimationSequence;
class USkeletonAsset;

class FAnimationSequenceLoadService
{
public:
    explicit FAnimationSequenceLoadService(FResourceManager& InResourceManager);

    UAnimationSequence* Load(const FString& Path);
    UAnimationSequence* ImportFbxSource(
        const FString& Path,
        const TArray<USkeletonAsset*>* ImportedSkeletonsOverride = nullptr);

private:
    UAnimationSequence* LoadAnimationAsset(const FString& AssetPath);
    UAnimationSequence* LoadSiblingAnimationAsset(const FString& NormalizedPath);
    UAnimationSequence* LoadBinary(const FString& BinaryPath, const FString& CacheKey);
    UAnimationSequence* LoadSiblingImportedBinary(const FString& NormalizedPath);
    UAnimationSequence* ImportFbxSourceToBinary(
        const FString& NormalizedPath,
        const TArray<USkeletonAsset*>* ImportedSkeletonsOverride);
    UAnimationSequence* FinalizeLoadedSequence(FAnimationSequence* SequenceData, const FString& CacheKey);

    FResourceManager& ResourceManager;
};

