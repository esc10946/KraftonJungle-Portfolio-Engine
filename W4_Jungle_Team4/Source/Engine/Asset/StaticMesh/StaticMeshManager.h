#pragma once

#include "Core/CoreTypes.h"
#include "Core/Singleton.h"

struct FStaticMeshAsset;

class FStaticMeshManager : public TSingleton<FStaticMeshManager>
{
	friend class TSingleton<FStaticMeshManager>;

public:
	// 파일 경로 -> static mesh asset 로드
	FStaticMeshAsset* LoadStaticMeshAsset(const FString& InSourcePath);

private:
	FStaticMeshManager() = default;
};