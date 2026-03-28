#pragma once

#include "Core/CoreTypes.h"

struct FObjInfo;
struct FStaticMeshAsset;

// parser가 만든 raw 데이터(FObjInfo)를 엔진이 사용할 cooked 데이터(FStaticMeshAsset)로 바꾸는 역할
class FObjImporter
{
public:
	static bool ImportFromFile(const FString& InObjPath, FStaticMeshAsset& OutAsset);
	// raw 데이터를 cooked asset으로 변환
	static bool BuildStaticMeshAsset(const FObjInfo& InObjInfo, FStaticMeshAsset& OutAsset);
};