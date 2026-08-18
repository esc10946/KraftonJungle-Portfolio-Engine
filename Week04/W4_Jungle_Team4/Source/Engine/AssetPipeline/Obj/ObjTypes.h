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
	TArray<FObjIndexTriplet> Vertices; // face는 삼각형, 사각형, ..일 수 있음 -> 꼭짓점 배열로 관리 
	FString MaterialName; // face 하나가 어떤 material을 쓸지
};

// .mtl에서 읽는 재질 1개 정보
struct FObjMaterialInfo
{
    FString Name;
	FString DiffuseTexturePath; // map_Kd ###.png

    FVector Ka = FVector(0, 0, 0);
    FVector Kd = FVector(1, 1, 1);
    FVector Ks = FVector(0, 0, 0);
    FVector Ke = FVector(0, 0, 0);

    float Ns = 0.0f;
    float d = 1.0f;
    int32 illum = 0;

    FString map_Ka;
    FString map_Kd;
    FString map_Ks;
    FString map_Ns;
    FString map_d;
    FString map_Bump;
};

// .obj 전체 읽은 raw 결과물.
struct FObjInfo
{
	FString SourcePath;
	FString MaterialLibraryPath;

	TArray<FVector> Positions; // v
	TArray<FVector2> TexCoords; // vt
	TArray<FVector> Normals; // vn
	TArray<FObjFace> Faces; // f

	// key = material name
	TMap<FString, FObjMaterialInfo> Materials;

	void Reset()
	{
		SourcePath.clear();
		MaterialLibraryPath.clear();

		Positions.clear();
		TexCoords.clear();
		Normals.clear();
		Faces.clear();
		Materials.clear();
	}
};

