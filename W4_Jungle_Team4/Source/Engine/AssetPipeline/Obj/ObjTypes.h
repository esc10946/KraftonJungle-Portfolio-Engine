#pragma once

#include "Core/CoreTypes.h"
#include "Math/Vector.h"

// OBJ의 f position / uv / normal 인덱스
struct FObjIndexTriplet
{
	int32 PositionIndex = -1;
	int32 TexCoordIndex = -1;
	int32 NormalIndex = -1;
};

// OBJ의 f 한 줄.
struct FObjFace
{
	TArray<FObjIndexTriplet> Vertices;
	FString MaterialName;
};

// .mtl에서 읽는 재질 1개 정보
struct FObjMaterialInfo
{
	FString Name;
	FVector DiffuseColor = FVector(1.0f, 1.0f, 1.0f);
	FString DiffuseTexturePath;
};

// .obj 전체 읽은 raw 결과물.
struct FObjInfo
{
	FString SourcePath;
	FString MaterialLibraryPath;

	TArray<FVector> Positions; // v
	TArray<FVector2> TexCoords; // vt
	TArray<FVector> Normals; // vn
	TArray<FObjFace> Faces;

	TMap<FString, FObjMaterialInfo> Materials;

	void Reset()
	{
		SourcePath.clear();
		MaterialLibraryPath.clear();

		Positions.clear();
		TexCoords.clear();
		Normals.clear();
		Faces.clear();
	}
};

