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

	FString Line;
	while (std::getline(File, Line))
	{
		// 각 줄을 개별 파싱 함수에 넘기기
		// 파싱 실패 발생 -> 전체 실패 처리
		if (!ParseLine(Line, OutObjInfo))
		{
			return false;
		}
	}
	return true;
}

bool FObjParser::ParseLine(const FString& InLine, FObjInfo& InOutObjInfo)
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
	const float V = std::stof(InTokens[2]);

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
