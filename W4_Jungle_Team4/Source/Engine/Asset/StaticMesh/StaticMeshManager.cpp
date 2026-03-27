#include "Asset/StaticMesh/StaticMeshManager.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "AssetPipeline/Obj/ObjImporter.h"

FStaticMeshAsset* FStaticMeshManager::LoadStaticMeshAsset(const FString& InSourcePath)
{
	// 최종 결과물 담을 asset
	FStaticMeshAsset* NewAsset = new FStaticMeshAsset();

	// raw -> cooked 변환
	if (!FObjImporter::ImportFromFile(InSourcePath, *NewAsset))
	{
		// import 실패 시 asset 정리
		delete NewAsset;
		return nullptr;
	}

	return NewAsset;
}