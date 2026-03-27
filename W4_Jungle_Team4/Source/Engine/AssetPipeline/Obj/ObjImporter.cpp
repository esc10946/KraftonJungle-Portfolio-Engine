#include "AssetPipeline/Obj/ObjImporter.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "AssetPipeline/Obj/ObjParser.h"
#include "AssetPipeline/Obj/ObjTypes.h"

bool FObjImporter::ImportFromFile(const FString& InObjPath, FStaticMeshAsset& OutAsset)
{
	// OBJ 텍스트 읽고 raw 데이터 만들기
	FObjInfo ObjInfo;
	if (!FObjParser::ParseFile(InObjPath, ObjInfo))
	{
		return false;
	}

	// raw 데이터 -> cooked 데이터
	return BuildStaticMeshAsset(ObjInfo, OutAsset);
}

bool FObjImporter::BuildStaticMeshAsset(const FObjInfo& InObjInfo, FStaticMeshAsset& OutAsset)
{
	OutAsset.SourcePath = InObjInfo.SourcePath;
	return true;
}
