#pragma once

#include "Core/CoreTypes.h"
#include "AssetPipeline/Obj/ObjTypes.h"

class FObjParser
{
public:
	// 파일 전체 읽어 FObjInfo 채우기
	static bool ParseFile(const FString& InObjPath, FObjInfo& OutObjInfo);

private:
	// 한 줄을 읽어 ParsePositon/ParseTexCoord/ParseNormal 로 넘김
	static bool ParseLine(const FString& InLine, FObjInfo& InOutObjInfo);
	// v x y z
	static bool ParsePosition(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo);
	// vt u v
	static bool ParseTexCoord(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo);
	// vn x y z 
	static bool ParseNormal(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo);
	static bool ParseFace(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, FObjInfo& InOutObjInfo);
	static bool ParseUseMaterial(const TArray<FString>& InTokens, FString& InOutCurrentMaterialName);
	static bool ParseMaterialLibrary(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo);
	static bool ParseIndexTriplet(const FString& InToken, FObjFace& InOutFace);
};
