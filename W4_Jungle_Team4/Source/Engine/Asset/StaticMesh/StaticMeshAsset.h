#pragma once

#include "Core/CoreTypes.h"
#include "Math/Vector.h"

/**
@brief 최종 렌더링에 들어갈 정점 1개 데이터.
*/
struct FStaticMeshVertex
{
	FVector Position;
	FVector Normal;
	FVector2 TexCoord;
};

/**
@brief 메시를 머티리얼 단위로 나눈 구간 정보
*/
struct FStaticMeshSection
{
	uint32 FirstIndex = 0;
	uint32 IndexCount = 0;
	FString MaterialName;
};


/**
 * @brief static mesh 전체를 담는 cooked 결과.
 * Vertices, Indices, Sections, 원본 경로 한 덩어리로 들고 있는 최종 asset
 */
struct FStaticMeshAsset
{
	FString SourcePath;
	FString BakePath;

	TArray<FStaticMeshVertex> Vertices;
	TArray<uint32> Indices;
	TArray<FStaticMeshSection> Sections;
};
