#pragma once

#include "Core/CoreTypes.h"

struct FStaticMeshAsset;

class FStaticMeshBakeIO
{
public:
	// obj 경로를 기준으로 bake 파일 경로를 만든다.
	static FString MakeBakePath(const FString& InSourcePath);

	// bake 파일이 원본 obj보다 최신인지 확인한다.
	static bool IsBakeUpToDate(const FString& InSourcePath, const FString& InBakePath);

	// cooked asset를 binary로 저장한다.
	static bool SaveToFile(const FString& InBakePath, const FStaticMeshAsset& InAsset);

	// binary bake 파일을 읽어 cooked asset를 복원한다.
	static bool LoadFromFile(const FString& InBakePath, FStaticMeshAsset& OutAsset);
};
