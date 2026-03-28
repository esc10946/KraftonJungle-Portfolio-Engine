#pragma once

#include "Core/CoreTypes.h"

class UStaticMesh;
struct FStaticMeshAsset;

// 발제 호환용 OBJ 전용 façade
// 내부 구현은 FStaticMeshManager에 위임한다.
class FObjManager
{
public:
	static UStaticMesh* LoadOBJ(const FString& InObjPath);

	static FStaticMeshAsset* LoadObjStaticMeshAsset(const FString& InObjPath);
	static UStaticMesh* LoadObjStaticMesh(const FString& InObjPath);

	static void Preload(const TArray<FString>& InObjPaths);
};