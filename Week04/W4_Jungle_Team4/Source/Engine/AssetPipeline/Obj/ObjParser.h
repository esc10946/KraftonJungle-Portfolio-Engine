#pragma once

#include "Core/CoreTypes.h"
#include "AssetPipeline/Obj/ObjTypes.h"

struct FObjFace;
struct FObjInfo;

class FObjParser
{
public:
	// 파일 전체 읽어 FObjInfo 채우기
	static bool ParseFile(const FString& InObjPath, FObjInfo& OutObjInfo);

private:
	// 한 줄을 읽어 ParsePositon/ParseTexCoord/ParseNormal 로 넘김
	static bool ParseLine(const FString& InLine, FString& InOutCurrentMaterialName, FObjInfo& InOutObjInfo);
	// v x y z
	static bool ParsePosition(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo);
	// vt u v
	static bool ParseTexCoord(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo);
	// vn x y z 
	static bool ParseNormal(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo);
	// f a/b/c d/e/f ...
	static bool ParseFace(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, FObjInfo& InOutObjInfo);
	// 12/3/4 같은 토큰 하나를 positon/uv/normal 인덱스로 분해
	static bool ParseIndexTriplet(const FString& InToken, FObjFace& InOutFace);

	static bool ParseUseMaterial(const TArray<FString>& InTokens, FString& InOutCurrentMaterialName);
	static bool ParseMaterialLibrary(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo);
};
