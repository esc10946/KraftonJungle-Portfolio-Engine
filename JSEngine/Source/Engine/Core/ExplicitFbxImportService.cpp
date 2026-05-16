#include "Core/ExplicitFbxImportService.h"

#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"

FExplicitFbxImportService::FExplicitFbxImportService(FResourceManager& InResourceManager)
    : ResourceManager(InResourceManager)
{
}

TArray<FImportedFbxAssetRecord> FExplicitFbxImportService::Import(const FString& SourceFbxPath)
{
    const FString NormalizedSourcePath = FPaths::Normalize(SourceFbxPath);

    ResourceManager.ImportSkeletonsFromFbx(NormalizedSourcePath);
    ResourceManager.LoadSkeletalMesh(NormalizedSourcePath);
    ResourceManager.LoadAnimationSequence(NormalizedSourcePath);

    FImportedFbxAssetDiscovery Discovery;
    TArray<FImportedFbxAssetRecord> Records = Discovery.DiscoverForSourceFbx(NormalizedSourcePath);

    UE_LOG("[ExplicitFbxImport] Imported | Source=%s | Records=%zu",
           NormalizedSourcePath.c_str(),
           Records.size());

    return Records;
}
