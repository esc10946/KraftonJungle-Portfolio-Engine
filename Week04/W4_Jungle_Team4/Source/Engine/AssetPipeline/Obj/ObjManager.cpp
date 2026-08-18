#include "AssetPipeline/Obj/ObjManager.h"

#include "Asset/StaticMesh/StaticMeshManager.h"

UStaticMesh* FObjManager::LoadOBJ(const FString& InObjPath)
{
	return LoadObjStaticMesh(InObjPath);
}

FStaticMeshAsset* FObjManager::LoadObjStaticMeshAsset(const FString& InObjPath)
{
	return FStaticMeshManager::Get().LoadStaticMeshAsset(InObjPath);
}

UStaticMesh* FObjManager::LoadObjStaticMesh(const FString& InObjPath)
{
	return FStaticMeshManager::Get().LoadStaticMeshObject(InObjPath);
}

void FObjManager::Preload(const TArray<FString>& InObjPaths)
{
	FStaticMeshManager::Get().Preload(InObjPaths);
}
