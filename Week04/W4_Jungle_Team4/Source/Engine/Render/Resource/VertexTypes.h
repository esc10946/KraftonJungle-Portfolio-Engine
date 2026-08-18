#pragma once

#include "Math/Vector.h"

/*
	Vertex 구조체들을 정의하는 Header입니다.
	추후에 다양한 Vertex 구조체들을 추가할 수 있습니다.
*/

struct FVertex
{ 
	FVector Position;
	FVector4 Color;
	int SubID;
};

struct FOverlayVertex
{
	float X, Y;
};


// Position + TexCoord 범용 버텍스 (FontBatcher, SubUVBatcher 등 텍스처 기반 배처 공용)
struct FTextureVertex
{
	FVector  Position;
	FVector  Normal = FVector(0.0f, 0.0f, 1.0f);
	FVector2 TexCoord;

	FTextureVertex() = default;
	FTextureVertex(const FVector& InPosition, const FVector2& InTexCoord)
		: Position(InPosition), TexCoord(InTexCoord)
	{
	}

	FTextureVertex(const FVector& InPosition, const FVector& InNormal, const FVector2& InTexCoord)
		: Position(InPosition), Normal(InNormal), TexCoord(InTexCoord)
	{
	}
};

struct FMeshData
{
	TArray<FVertex> Vertices;
	TArray<uint32> Indices;
};

struct FTexturedMeshData
{
	TArray<FTextureVertex> Vertices;
	TArray<uint32> Indices;
};
