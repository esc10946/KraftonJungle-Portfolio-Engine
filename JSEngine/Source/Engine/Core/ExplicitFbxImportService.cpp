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
    const FFbxMeshContentInfo ContentInfo = ResourceManager.InspectFbxMeshContent(NormalizedSourcePath);

    if (ContentInfo.bHasStaticMesh || ContentInfo.bHasSkeletalMesh)
    {
        ResourceManager.ImportMaterialFromFbx(NormalizedSourcePath);
    }

    if (ContentInfo.bHasStaticMesh)
    {
        ResourceManager.ImportStaticMeshFromFbx(NormalizedSourcePath);
    }

    if (ContentInfo.bHasSkeletalMesh)
    {
        ResourceManager.ImportSkeletonsFromFbx(NormalizedSourcePath);
        ResourceManager.ImportSkeletalMeshFromFbx(NormalizedSourcePath);
        ResourceManager.ImportAnimationSequencesFromFbx(NormalizedSourcePath);
    }

    FImportedFbxAssetDiscovery Discovery;
    TArray<FImportedFbxAssetRecord> Records = Discovery.DiscoverForSourceFbx(NormalizedSourcePath);

    UE_LOG("[ExplicitFbxImport] Imported | Source=%s | Records=%zu",
           NormalizedSourcePath.c_str(),
           Records.size());

    return Records;
}
