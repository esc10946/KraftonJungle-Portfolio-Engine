#pragma once

#include "Core/CoreTypes.h"

struct FObjMaterialInfo;

class FMtlParser
{
public:
	// .mtl 읽어 material map 채우기
	static bool ParseFile(const FString& InMtlPath, TMap<FString, FObjMaterialInfo>& OutMaterials);

private:
	static bool ParseLine(const FString& InLine, FString& InOutCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials);
	// newmtl MaterialName
	static bool ParseNewMaterial(const TArray<FString>& InTokens, FString& InOutCurrentMatrialName, TMap<FString, FObjMaterialInfo>& InOutMaterials);
	// Kd r g b
	static bool ParseDiffuseColor(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials);
	// map_Kd ###.png
	static bool ParseDiffuseTexture(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials);
};