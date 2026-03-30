#include "AssetPipeline/Obj/ObjParser.h"

#include <fstream>
#include <sstream>

namespace
{
	// 한 줄 문자열을 공백 기준으로 잘라서 토큰 배열로 만드는 헬퍼 함수
	// ex) v 1.0 2.0 3.0 -> ["v", "1.0", "2.0", "3.0"]
	TArray<FString> Tokenize(const FString& InLine)
	{
		TArray<FString> Tokens;
		std::stringstream Stream(InLine); // 내부적으로 InLIne = [v][ ][1][.][0][ ][2][.][0][ ][3][.][0]

		FString Token;
		while (Stream >> Token) // 공백 처리
		{
			Tokens.push_back(Token);
		}

		return Tokens;
	}
}

bool FObjParser::ParseFile(const FString& InObjPath, FObjInfo& OutObjInfo)
{
	// OBJ 파일 열기
	std::ifstream File(InObjPath);
	
	if (!File.is_open()) 
	{
		return false;
	}

	// 이전에 사용한 데이터 남아있으면 초기화
	OutObjInfo.Reset();
	// 어떤 파일에서 읽었는지 기록
	OutObjInfo.SourcePath = InObjPath;

	// usemtl은 '현재 상태'를 기억해두고, 이후 facae에 붙여주는 방식
	FString CurrentMaterialName;

	FString Line;
	while (std::getline(File, Line))
	{
		if (!Line.empty() && Line.back() == '\r')
		{
			Line.pop_back();
		}
		// 각 줄을 개별 파싱 함수에 넘기기
		// 파싱 실패 발생 -> 전체 실패 처리
		if (!ParseLine(Line, CurrentMaterialName, OutObjInfo))
		{
			return false;
		}
	}
	return true;
}

bool FObjParser::ParseLine(const FString& InLine, FString& InOutCurrentMaterialName, FObjInfo& InOutObjInfo)
{
	if (InLine.empty()) // 빈 줄 무시
	{
		return true;
	}
	if (InLine[0] == '#') // 주석 무시
	{
		return true;
	}

	const TArray<FString> Tokens = Tokenize(InLine);
	if (Tokens.empty())
	{
		return true;
	}

	const FString& Prefix = Tokens[0];
	
	if (Prefix == "v") // "v" 처리
	{
		return ParsePosition(Tokens, InOutObjInfo);
	}
	if (Prefix == "vt") // "vt" 처리
	{
		return ParseTexCoord(Tokens, InOutObjInfo);
	}
	if (Prefix == "vn") // "vn" 처리
	{
		return ParseNormal(Tokens, InOutObjInfo);
	}
	if (Prefix == "f")
	{
		return ParseFace(Tokens, InOutCurrentMaterialName, InOutObjInfo);
	}
	if (Prefix == "usemtl")
	{
		return ParseUseMaterial(Tokens, InOutCurrentMaterialName);
	}
	if (Prefix == "mtllib")
	{
		return ParseMaterialLibrary(Tokens, InOutObjInfo);
	}

	return true;
}

bool FObjParser::ParsePosition(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo)
{
	// 최소 4개 토큰으로 이루어져야 함 (v x y z)
	if (InTokens.size() < 4) return false;

	// 문자열을 float로 변환
	const float X = std::stof(InTokens[1]);
	const float Y = std::stof(InTokens[2]);
	const float Z = std::stof(InTokens[3]);

	// 엔진 벡터 타입으로 만든 뒤 Positions 배열에 저장
	InOutObjInfo.Positions.emplace_back(X, Y, Z);
	return true;
}

bool FObjParser::ParseTexCoord(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo)
{
	if (InTokens.size() < 3) return false;

	const float U = std::stof(InTokens[1]);
	const float V = 1.0f - std::stof(InTokens[2]);

	InOutObjInfo.TexCoords.emplace_back(U, V);
	return true; 
}

bool FObjParser::ParseNormal(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo)
{
	if (InTokens.size() < 4) return false;

	const float X = std::stof(InTokens[1]);
	const float Y = std::stof(InTokens[2]);
	const float Z = std::stof(InTokens[3]);

	InOutObjInfo.Normals.emplace_back(X, Y, Z);
	return true;
}

bool FObjParser::ParseFace(const TArray<FString>& InTokens, const FString& InCurrentMaterialName, FObjInfo& InOutObjInfo)
{
	if (InTokens.size() < 4) return false;

	FObjFace Face;
	Face.MaterialName = InCurrentMaterialName;

	// f 시작이므로 1부터 시작
	for (size_t Index = 1; Index < InTokens.size(); ++Index)
	{
		if (!ParseIndexTriplet(InTokens[Index], Face))
		{
			return false;
		}
	}

	InOutObjInfo.Faces.push_back(Face);
	return true;
}

bool FObjParser::ParseIndexTriplet(const FString& InToken, FObjFace& InOutFace)
{
	FObjIndexTriplet Triplet;

	// 3/7/2 
	// 3//2
	// 3/7 -> "/" 기준으로 최대 3개 읽기
	size_t Start = 0;
	size_t SlashIndex = InToken.find('/');
	int ComponentIndex = 0; // 0 = position, 1 = uv, 2 = normal

	while (Start != FString::npos && ComponentIndex < 3)
	{
		// ex) 3/7/2
		// start 0, slashindex 1, part 3
		// start 2, slashindex 3, part 7
		// start 4, slashindex npos, part 2
		const FString Part = (SlashIndex == FString::npos)
			? InToken.substr(Start)
			: InToken.substr(Start, SlashIndex - Start);

		if (!Part.empty())
		{
			// (-1) : OBJ의 1 -> 배열의 0
			const int32 Value = std::stoi(Part) - 1;

			if (ComponentIndex == 0)
			{
				Triplet.PositionIndex = Value;
			}
			else if (ComponentIndex == 1)
			{
				Triplet.TexCoordIndex = Value;
			}
			else if (ComponentIndex == 2)
			{
				Triplet.NormalIndex = Value;
			}
		}

		if (SlashIndex == FString::npos) break;
	
		Start = SlashIndex + 1;
		SlashIndex = InToken.find('/', Start);
		++ComponentIndex;
	}

	InOutFace.Vertices.push_back(Triplet);
	return true;
}

bool FObjParser::ParseUseMaterial(const TArray<FString>& InTokens, FString& InOutCurrentMaterialName)
{
	// usemtl MaterialName
	if (InTokens.size() < 2) return false;

	InOutCurrentMaterialName = InTokens[1];
	return true;
}

bool FObjParser::ParseMaterialLibrary(const TArray<FString>& InTokens, FObjInfo& InOutObjInfo)
{
	// mtllib ###.mtl
	if (InTokens.size() < 2) return false;

	InOutObjInfo.MaterialLibraryPath = InTokens[1];
	return true;
}
