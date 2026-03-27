#include "AssetPipeline/Obj/ObjImporter.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "AssetPipeline/Obj/ObjParser.h"
#include "AssetPipeline/Obj/ObjTypes.h"
#include "AssetPipeline/Obj/MtlParser.h"

#include <filesystem>

bool FObjImporter::ImportFromFile(const FString& InObjPath, FStaticMeshAsset& OutAsset)
{
	// OBJ 텍스트 읽고 raw 데이터 만들기
	FObjInfo ObjInfo;
	if (!FObjParser::ParseFile(InObjPath, ObjInfo))
	{
		return false;
	}

	// mtllib 있으면 mtl 파일 읽어서 Materials 채우기
	const FString MaterialLibraryPath = ResolveMaterialLibraryPath(ObjInfo);
	if (!MaterialLibraryPath.empty())
	{
		FMtlParser::ParseFile(MaterialLibraryPath, ObjInfo.Materials);
		// 여기서는 mtl parse 실패를 치명적 에러로 보지 않아도 된다.
		// 필요하면 나중에 정책을 바꿀 수 있다.
	}

	// raw 데이터 -> cooked 데이터
	return BuildStaticMeshAsset(ObjInfo, OutAsset);
}

bool FObjImporter::BuildStaticMeshAsset(const FObjInfo& InObjInfo, FStaticMeshAsset& OutAsset)
{
	OutAsset.SourcePath = InObjInfo.SourcePath;
	OutAsset.BakePath.clear();
	OutAsset.Vertices.clear();
	OutAsset.Indices.clear();
	OutAsset.Sections.clear();

	if (!BuildGeometry(InObjInfo, OutAsset)) return false;
	
	BuildSections(InObjInfo, OutAsset);

	return !OutAsset.Vertices.empty();
}

bool FObjImporter::TriangulateFace(const FObjFace& InFace, TArray<FObjFace>& OutTriangles)
{
	// triangle 안 되는 face 무시
	if (InFace.Vertices.size() < 3) return false;

	// fan triangulation
	// [0, 1, 2, 3] -> (0, 1, 2), (0, 2, 3)
	for (size_t Index = 1; Index + 1 < InFace.Vertices.size(); ++Index)
	{
		FObjFace Triangle;
		Triangle.MaterialName = InFace.MaterialName;
		
		Triangle.Vertices.push_back(InFace.Vertices[0]);
		Triangle.Vertices.push_back(InFace.Vertices[Index]);
		Triangle.Vertices.push_back(InFace.Vertices[Index+1]);

		OutTriangles.push_back(Triangle);
	}

	return true;
}

bool FObjImporter::BuildGeometry(const FObjInfo& InObjInfo, FStaticMeshAsset& InOutAsset)
{
	for (const FObjFace& Face : InObjInfo.Faces)
	{
		TArray<FObjFace> Triangles;
		if (!TriangulateFace(Face, Triangles)) continue;

		for (const FObjFace& Triangle : Triangles)
		{
			for (const FObjIndexTriplet& IndexTriplet : Triangle.Vertices)
			{
				// Position Index
				if (IndexTriplet.PositionIndex < 0 ||
					IndexTriplet.PositionIndex >= static_cast<int32>(InObjInfo.Positions.size()))
				{
					return false;
				}
				
				FStaticMeshVertex Vertex;

				Vertex.Position = InObjInfo.Positions[IndexTriplet.PositionIndex];
			
				// Normal은 있으면 채우고, 없으면 기본값 유지
				if (IndexTriplet.NormalIndex >= 0 &&
					IndexTriplet.NormalIndex < static_cast<int32>(InObjInfo.Normals.size()))
				{
					Vertex.Normal = InObjInfo.Normals[IndexTriplet.NormalIndex];
				}

				// UV도 있으면 채우고, 없으면 기본값 유지
				if (IndexTriplet.TexCoordIndex >= 0 &&
					IndexTriplet.TexCoordIndex < static_cast<int32>(InObjInfo.TexCoords.size()))
				{
					Vertex.TexCoord = InObjInfo.TexCoords[IndexTriplet.TexCoordIndex];
				}

				InOutAsset.Vertices.push_back(Vertex);
				InOutAsset.Indices.push_back(static_cast<uint32>(InOutAsset.Vertices.size() - 1));
			}
		}
	}

	return !InOutAsset.Vertices.empty();
}

void FObjImporter::BuildSections(const FObjInfo& InObjInfo, FStaticMeshAsset& InOutAsset)
{
	InOutAsset.Sections.clear();

	FString CurrentMaterialName;
	FStaticMeshSection* CurrentSection = nullptr;
	uint32 RunningIndex = 0;

	for (const FObjFace& Face : InObjInfo.Faces)
	{
		// face가 N각형이면 triangle은 N-2개
		const uint32 TriangleCount = Face.Vertices.size() >= 3
			? static_cast<uint32>(Face.Vertices.size() - 2) : 0;

		const uint32 FaceIndexCount = TriangleCount * 3;
		if (FaceIndexCount == 0)
		{
			continue;
		}

		// material이 바뀌면 새로운 section 시작
		if (!CurrentSection || CurrentMaterialName != Face.MaterialName)
		{
			FStaticMeshSection Section;
			Section.FirstIndex = RunningIndex;
			Section.MaterialName = Face.MaterialName;

			InOutAsset.Sections.push_back(Section);
			CurrentSection = &InOutAsset.Sections.back();
			CurrentMaterialName = Face.MaterialName;
		}

		CurrentSection->IndexCount += FaceIndexCount;
		RunningIndex += FaceIndexCount;
	}
}

FString FObjImporter::ResolveMaterialLibraryPath(const FObjInfo& InObjInfo)
{
	if (InObjInfo.MaterialLibraryPath.empty())
	{
		return FString();
	}

	const std::filesystem::path MaterialPath =
		std::filesystem::path(InObjInfo.SourcePath).parent_path() / InObjInfo.MaterialLibraryPath;

	return MaterialPath.lexically_normal().string();
}