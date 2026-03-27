#pragma once

#include "Core/CoreTypes.h"
#include "Math/Vector.h"

struct FMeshData;

// OBJ import가 끝난 뒤 엔진이 공통적으로 소비하게 될 정점 포맷입니다.
// 현재 렌더러가 Position/Color 위주로 동작하더라도, 이후 multiple material,
// lighting, UV sampling 확장을 고려해 normal / texcoord까지 포함합니다.
struct FStaticMeshVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	FVector2 TexCoord;
};

// 하나의 StaticMesh 안에서 material 단위로 잘린 draw 구간입니다.
// 예를 들어 OBJ에서 usemtl이 2번 바뀌었다면 Section도 2개 이상 생길 수 있습니다.
struct FStaticMeshSection
{
	// Indices 배열 기준 시작 위치입니다.
	uint32 FirstIndex = 0;

	// 이 Section이 사용하는 index 개수입니다.
	uint32 IndexCount = 0;

	// material 슬롯 이름 또는 원본 mtl 이름입니다.
	// 다른 팀원이 UMaterial 시스템을 붙일 때 slot lookup key로 사용할 수 있습니다.
	FString MaterialName;
};

// OBJ/MTL raw 데이터를 엔진용으로 정리한 cooked mesh 데이터입니다.
// 이 구조체 자체는 UObject가 아니며, 순수 데이터 컨테이너 역할만 담당합니다.
// 런타임에서는 UStaticMesh가 이 데이터를 감싸는 wrapper가 되기를 기대합니다.
struct FStaticMeshAsset
{
	// 원본 OBJ 파일 경로입니다.
	FString SourcePath;

	// bake 결과 파일 경로입니다.
	FString BakePath;

	// 렌더/충돌/가공에 사용하는 정점/인덱스 버퍼입니다.
	TArray<FStaticMeshVertex> Vertices;
	TArray<uint32> Indices;

	// multiple material 지원을 위한 section 목록입니다.
	TArray<FStaticMeshSection> Sections;

	// 간단한 bounds 정보입니다.
	FVector BoundsMin;
	FVector BoundsMax;

	// Asset을 빈 상태로 되돌립니다.
	// 재사용 가능한 parser/importer가 같은 인스턴스를 여러 번 채울 때 사용합니다.
	void Reset();

	// 현재 asset이 유효한 렌더 데이터를 가지고 있는지 빠르게 확인합니다.
	// 보통 vertex/index 중 하나라도 비어 있으면 false가 됩니다.
	bool IsValid() const;

	// 현재 cooked 데이터를 기존 렌더러가 이해하는 FMeshData 포맷으로 복사/변환합니다.
	// 현 렌더러가 normal/uv를 직접 쓰지 못하더라도, 이 함수가 협업용 브릿지 지점이 됩니다.
	bool BuildRenderMeshData(FMeshData& OutMeshData) const;

	// BoundsMin / BoundsMax를 Vertices 기준으로 다시 계산합니다.
	// importer 구현 시 마지막 단계에서 호출하는 것을 기대합니다.
	void UpdateBounds();
};

// 과제 문서에서 FStaticMesh라는 이름을 쓰는 경우가 있어 협업 혼선을 줄이기 위한 별칭입니다.
using FStaticMesh = FStaticMeshAsset;
