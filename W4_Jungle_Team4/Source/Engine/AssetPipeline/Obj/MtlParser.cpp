#include "AssetPipeline/Obj/MtlParser.h"
#include "AssetPipeline/Obj/ObjTypes.h"

#include <fstream>
#include <sstream>

namespace
{
	TArray<FString> Tokenize(const FString& InLine)
	{
		TArray<FString> Tokens;
		std::stringstream Stream(InLine);

		FString Token;
		while (Stream >> Token)
		{
			Tokens.push_back(Token);
		}

		return Tokens;
	}
}

bool FMtlParser::ParseFile(const FString& InMtlPath, TMap<FString, FObjMaterialInfo>& OutMaterials)
{
	std::ifstream File(InMtlPath);
	if (!File.is_open())
	{
		return false;
	}

	// 새로 읽을 때는 기존 material map 비움
	OutMaterials.clear();

	FString CurrentMaterialName;
	FString Line;

	while (std::getline(File, Line))
	{
		if (!Line.empty() && Line.back() == '\r')
		{
			Line.pop_back();
		}

		if (!ParseLine(Line, CurrentMaterialName, OutMaterials))
		{
			return false;
		}
	}

	return true;
}

bool FMtlParser::ParseLine(
	const FString& InLine,
	FString& InOutCurrentMaterialName,
	TMap<FString, FObjMaterialInfo>& InOutMaterials)
{
	if (InLine.empty())
	{
		return true;
	}

	if (InLine[0] == '#')
	{
		return true;
	}

	const TArray<FString> Tokens = Tokenize(InLine);
	if (Tokens.empty())
	{
		return true;
	}

	const FString& Prefix = Tokens[0];

	if (Prefix == "newmtl")
	{
		return ParseNewMaterial(Tokens, InOutCurrentMaterialName, InOutMaterials);
	}

	if (Prefix == "Ka")
    {
        return ParseVec3Property(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::Ka);
    }

    if (Prefix == "Kd")
    {
        return ParseVec3Property(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::Kd);
    }

    if (Prefix == "Ks")
    {
        return ParseVec3Property(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::Ks);
    }

    if (Prefix == "Ke")
    {
        return ParseVec3Property(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::Ke);
    }

    if (Prefix == "Ns")
    {
        return ParseFloatProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::Ns);
    }

    if (Prefix == "d")
    {
        return ParseFloatProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::d);
    }

    if (Prefix == "illum")
    {
        return ParseIntProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::illum);
    }

    if (Prefix == "map_Ka")
    {
        return ParseStringProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::map_Ka);
    }

    if (Prefix == "map_Kd")
    {
        return ParseStringProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::map_Kd);
    }

    if (Prefix == "map_Ks")
    {
        return ParseStringProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::map_Ks);
    }

    if (Prefix == "map_Ns")
    {
        return ParseStringProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::map_Ns);
    }

    if (Prefix == "map_d")
    {
        return ParseStringProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::map_d);
    }

    if (Prefix == "map_bump" || Prefix == "bump")
    {
        return ParseStringProperty(Tokens, InOutCurrentMaterialName, InOutMaterials, &FObjMaterialInfo::map_Bump);
    }


	// 나머지 mtl 속성은 아직 무시
	return true;
}

bool FMtlParser::ParseNewMaterial(
	const TArray<FString>& InTokens,
	FString& InOutCurrentMaterialName,
	TMap<FString, FObjMaterialInfo>& InOutMaterials)
{
	if (InTokens.size() < 2)
	{
		return false;
	}

	const FString& MaterialName = InTokens[1];
	InOutCurrentMaterialName = MaterialName;

	FObjMaterialInfo MaterialInfo;
	MaterialInfo.Name = MaterialName;

	InOutMaterials[MaterialName] = MaterialInfo;
	return true;
}

bool FMtlParser::ParseVec3Property(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials, FVector FObjMaterialInfo::* Member)
{
	if (InTokens.size() < 4)
    {
        return false;
    }

    FObjMaterialInfo* Material = GetCurrentMaterial(InCurrentMaterialName, InOutMaterials);
    if (Material == nullptr)
    {
        return false;
    }

    const float X = std::stof(InTokens[1]);
    const float Y = std::stof(InTokens[2]);
    const float Z = std::stof(InTokens[3]);

    Material->*Member = FVector(X, Y, Z);
    return true;
}

bool FMtlParser::ParseFloatProperty(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials, float FObjMaterialInfo::* Member)
{
    if (InTokens.size() < 2)
    {
        return false;
    }

    FObjMaterialInfo* Material = GetCurrentMaterial(InCurrentMaterialName, InOutMaterials);
    if (Material == nullptr)
    {
        return false;
    }

    Material->*Member = std::stof(InTokens[1]);
    return true;
}

bool FMtlParser::ParseIntProperty(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials, int32 FObjMaterialInfo::* Member)
{
    if (InTokens.size() < 2)
    {
        return false;
    }

    FObjMaterialInfo* Material = GetCurrentMaterial(InCurrentMaterialName, InOutMaterials);
    if (Material == nullptr)
    {
        return false;
    }

    Material->*Member = std::stoi(InTokens[1]);
    return true;
}

bool FMtlParser::ParseStringProperty(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, TMap<FString, FObjMaterialInfo>& InOutMaterials, FString FObjMaterialInfo::* Member)
{
    if (InTokens.size() < 2)
    {
        return false;
    }

    FObjMaterialInfo* Material = GetCurrentMaterial(InCurrentMaterialName, InOutMaterials);
    if (Material == nullptr)
    {
        return false;
    }

    Material->*Member = InTokens[1];
    return true;
}

FObjMaterialInfo* FMtlParser::GetCurrentMaterial(
    const FString& InCurrentMaterialName,
    TMap<FString, FObjMaterialInfo>& InOutMaterials)
{
    if (InCurrentMaterialName.empty())
    {
        return nullptr;
    }

    auto It = InOutMaterials.find(InCurrentMaterialName);
    if (It == InOutMaterials.end())
    {
        return nullptr;
    }

    return &It->second;
}