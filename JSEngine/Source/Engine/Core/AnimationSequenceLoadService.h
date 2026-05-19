#pragma once

#include "Asset/AnimationTypes.h"
#include "Core/CoreMinimal.h"

class FResourceManager;
class UAnimationSequence;

class FAnimationSequenceLoadService
{
public:
    explicit FAnimationSequenceLoadService(FResourceManager& InResourceManager);

    UAnimationSequence* Load(const FString& Path);
    UAnimationSequence* ImportFbxSource(const FString& Path);

private:
    UAnimationSequence* LoadBinary(const FString& BinaryPath, const FString& CacheKey);
    UAnimationSequence* LoadSiblingImportedBinary(const FString& NormalizedPath);
    UAnimationSequence* ImportFbxSourceToBinary(const FString& NormalizedPath);
    UAnimationSequence* FinalizeLoadedSequence(FAnimationSequence* SequenceData, const FString& CacheKey);

    FResourceManager& ResourceManager;
};

