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

	if (Prefix == "Kd")
	{
		return ParseDiffuseColor(Tokens, InOutCurrentMaterialName, InOutMaterials);
	}

	if (Prefix == "map_Kd")
	{
		return ParseDiffuseTexture(Tokens, InOutCurrentMaterialName, InOutMaterials);
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

bool FMtlParser::ParseDiffuseColor(
	const TArray<FString>& InTokens,
	const FString& InCurrentMaterialName,
	TMap<FString, FObjMaterialInfo>& InOutMaterials)
{
	// current material이 없는데 Kd가 나오면 잘못된 mtl
	if (InCurrentMaterialName.empty())
	{
		return false;
	}

	if (InTokens.size() < 4)
	{
		return false;
	}

	const float R = std::stof(InTokens[1]);
	const float G = std::stof(InTokens[2]);
	const float B = std::stof(InTokens[3]);

	FObjMaterialInfo& MaterialInfo = InOutMaterials[InCurrentMaterialName];
	MaterialInfo.Name = InCurrentMaterialName;
	MaterialInfo.DiffuseColor = FVector(R, G, B);

	return true;
}

bool FMtlParser::ParseDiffuseTexture(
	const TArray<FString>& InTokens,
	const FString& InCurrentMaterialName,
	TMap<FString, FObjMaterialInfo>& InOutMaterials)
{
	if (InCurrentMaterialName.empty())
	{
		return false;
	}

	if (InTokens.size() < 2)
	{
		return false;
	}

	FObjMaterialInfo& MaterialInfo = InOutMaterials[InCurrentMaterialName];
	MaterialInfo.Name = InCurrentMaterialName;
	MaterialInfo.DiffuseTexturePath = InTokens[1];

	return true;
}
