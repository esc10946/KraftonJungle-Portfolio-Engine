#pragma once

#include "Core/CoreTypes.h"

struct FObjInfo;
struct FObjFace;
struct FStaticMeshAsset;

// parser가 만든 raw 데이터(FObjInfo)를 엔진이 사용할 cooked 데이터(FStaticMeshAsset)로 바꾸는 역할
class FObjImporter
{
public:
	static bool ImportFromFile(const FString& InObjPath, FStaticMeshAsset& OutAsset);
	// raw 데이터를 cooked asset으로 변환
	static bool BuildStaticMeshAsset(const FObjInfo& InObjInfo, FStaticMeshAsset& OutAsset);

private:
	// polygon face -> triangle list
	// 4개 -> triangle 2개
	// 5개 -> triangle 3개
	static bool TriangulateFace(const FObjFace& InFace, TArray<FObjFace>& OutTriangles);
	// raw face들 바탕으로 vertex/index 버퍼 생성
	static bool BuildGeometry(const FObjInfo& InObjInfo, FStaticMeshAsset& InOutAsset);
	static uint32 FindOrAddMaterialSlot(
		const FString& InMaterialName,
		const FObjInfo& InObjInfo,
		FStaticMeshAsset& InOutAsset,
		TMap<FString, uint32>& InOutSlotLookup);
	// usemtl 경계 기준으로 section 생성
	static void BuildSections(const FObjInfo& InObjInfo, FStaticMeshAsset& InOutAsset);

	// obj 파일 기준으로 mtllib 상대 경로를 실제 경로로 바꿈...?
	static FString ResolveMaterialLibraryPath(const FObjInfo& InObjInfo);
};
