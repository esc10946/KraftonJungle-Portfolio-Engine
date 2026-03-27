#pragma once

#include "Core/CoreTypes.h"

class FArchive;
struct FStaticMeshAsset;

// FStaticMeshAsset를 엔진 전용 binary bake 포맷으로 저장/로드하는 전용 유틸리티입니다.
// "무엇을 어떤 순서로 기록할지"를 정의하는 계층이며,
// 실제 파일 입출력은 FArchive / FWindowsBinWriter / FWindowsBinReader가 담당합니다.
class FStaticMeshBakeIO
{
public:
	// OBJ 경로를 기준으로 bake 파일 경로를 계산합니다.
	// 예: Assets/Mesh/Grass_block.obj -> Assets/Baked/Grass_block.smesh
	static FString MakeBakePath(const FString& InObjPath);

	// 디스크의 bake 파일이 원본 OBJ보다 최신인지 비교합니다.
	// manager는 이 결과를 보고 "재파싱 vs bake 재사용"을 결정하게 됩니다.
	static bool IsBakeUpToDate(const FString& InObjPath, const FString& InBakePath);

	// StaticMeshAsset를 bake 파일로 저장합니다.
	// 내부적으로 writer 생성 -> Save(FArchive&, Asset) 호출 흐름을 기대합니다.
	static bool SaveToFile(const FString& InBakePath, const FStaticMeshAsset& InAsset);

	// bake 파일에서 StaticMeshAsset를 복원합니다.
	// 내부적으로 reader 생성 -> Load(FArchive&, Asset) 호출 흐름을 기대합니다.
	static bool LoadFromFile(const FString& InBakePath, FStaticMeshAsset& OutAsset);

	// 이미 열린 archive 스트림에 asset을 직렬화합니다.
	// 테스트용 memory archive가 생기더라도 재사용할 수 있게 file I/O와 분리합니다.
	static bool Save(FArchive& Ar, const FStaticMeshAsset& InAsset);

	// archive에서 asset 데이터를 역직렬화합니다.
	// Save와 같은 필드 순서를 반드시 유지해야 합니다.
	static bool Load(FArchive& Ar, FStaticMeshAsset& OutAsset);
};
