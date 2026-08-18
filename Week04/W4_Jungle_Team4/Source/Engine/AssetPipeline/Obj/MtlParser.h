#pragma once

#include "Core/CoreTypes.h"
#include "Math/Vector.h"
#include "AssetPipeline/Obj/ObjTypes.h"

class FMtlParser
{
public:
	// .mtl 읽어 material map 채우기
	static bool ParseFile(const FString& InMtlPath, TMap<FString, FObjMaterialInfo>& OutMaterials);

private:
	static bool ParseLine(const FString& InLine, FString& InOutCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials);
	// newmtl MaterialName
	static bool ParseNewMaterial(const TArray<FString>& InTokens, FString& InOutCurrentMatrialName, TMap<FString, FObjMaterialInfo>& InOutMaterials);

	static bool ParseVec3Property(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials, FVector FObjMaterialInfo::*Member);
	static bool ParseFloatProperty(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials, float FObjMaterialInfo::*Member);
	static bool ParseIntProperty(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials, int32 FObjMaterialInfo::*Member);
	static bool ParseStringProperty(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials, FString FObjMaterialInfo::*Member);

	//현재 material 가져오는 공통 함수
	static FObjMaterialInfo* GetCurrentMaterial(
		const FString& InCurrentMaterialName,
		TMap<FString, FObjMaterialInfo>& InOutMaterials);
};