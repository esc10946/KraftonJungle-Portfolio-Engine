#include "AssetPipeline/Obj/ObjImporter.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "AssetPipeline/Obj/ObjParser.h"
#include "AssetPipeline/Obj/ObjTypes.h"
#include "AssetPipeline/Obj/MtlParser.h"
#include "Asset/Material/MaterialManager.h"

#include <filesystem>

#include <Core/Paths.h>

namespace
{
	static void ResolveSingleTexturePath(FString& InOutPath, const std::filesystem::path& MtlDirectory)
	{
		if (InOutPath.empty())
		{
			return;
		}

		std::filesystem::path TexturePath(InOutPath);

			// 이미 절대 경로면 그대로 사용
		if (!TexturePath.is_absolute())
		{
			TexturePath = MtlDirectory / TexturePath;
		}
		InOutPath = TexturePath.lexically_normal().generic_string();
	}

	void ResolveMaterialTexturePaths(const FString& InMtlPath, TMap<FString, FObjMaterialInfo>& InOutMaterials)
	{
		const std::filesystem::path MtlDirectory = 
			FPaths::TexturePath();


		for (auto& Pair : InOutMaterials)
		{
			FObjMaterialInfo& MaterialInfo = Pair.second;

			ResolveSingleTexturePath(MaterialInfo.map_Ka, MtlDirectory);
			ResolveSingleTexturePath(MaterialInfo.map_Kd, MtlDirectory);
			ResolveSingleTexturePath(MaterialInfo.map_Ks, MtlDirectory);
			ResolveSingleTexturePath(MaterialInfo.map_Ns, MtlDirectory);
			ResolveSingleTexturePath(MaterialInfo.map_d, MtlDirectory);
			ResolveSingleTexturePath(MaterialInfo.map_Bump, MtlDirectory);
		}

	}

	FVector ConvertYUpToZUp(const FVector& V)
	{
		return FVector(V.X, -V.Z, V.Y);
	}
}

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

		// map_Kd 상대 경로를 실제 파일 경로로 보정
		ResolveMaterialTexturePaths(MaterialLibraryPath, ObjInfo.Materials);
	}

	for (auto& pair : ObjInfo.Materials) {
		FMaterialManager::Get().CreateMaterial(pair.second);
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
	OutAsset.MaterialSlots.clear();
	OutAsset.Sections.clear();
	OutAsset.BoundsMin = FVector();
	OutAsset.BoundsMax = FVector();

	if (!BuildGeometry(InObjInfo, OutAsset)) return false;
	OutAsset.UpdateBounds();
	
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
	const bool bConvertYUpToZUp = InObjInfo.SourcePath.find("Minecraft_") != FString::npos;

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

				FVector Position = InObjInfo.Positions[IndexTriplet.PositionIndex];
				if (bConvertYUpToZUp)
				{
					Position = ConvertYUpToZUp(Position);
				}
				Vertex.Position = Position;

				if (IndexTriplet.NormalIndex >= 0 &&
					IndexTriplet.NormalIndex < static_cast<int32>(InObjInfo.Normals.size()))
				{
					FVector Normal = InObjInfo.Normals[IndexTriplet.NormalIndex];
					if (bConvertYUpToZUp)
					{
						Normal = ConvertYUpToZUp(Normal);
					}
					Vertex.Normal = Normal;
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

uint32 FObjImporter::FindOrAddMaterialSlot(
	const FString& InMaterialName,
	const FObjInfo& InObjInfo,
	FStaticMeshAsset& InOutAsset,
	TMap<FString, uint32>& InOutSlotLookup)
{
	const auto FoundIt = InOutSlotLookup.find(InMaterialName);
	if (FoundIt != InOutSlotLookup.end())
	{
		return FoundIt->second;
	}

	FStaticMaterialSlot Slot;
	Slot.SlotName = InMaterialName.empty()
		? FString("DefaultSlot") + std::to_string(InOutAsset.MaterialSlots.size())
		: InMaterialName;
	Slot.SourceMaterialName = InMaterialName;

	const auto MaterialIt = InObjInfo.Materials.find(InMaterialName);
	if (MaterialIt != InObjInfo.Materials.end())
	{
		Slot.DiffuseTexturePath = MaterialIt->second.map_Kd;
	}

	const uint32 NewSlotIndex = static_cast<uint32>(InOutAsset.MaterialSlots.size());
	InOutAsset.MaterialSlots.push_back(Slot);
	InOutSlotLookup[InMaterialName] = NewSlotIndex;
	return NewSlotIndex;
}

void FObjImporter::BuildSections(const FObjInfo& InObjInfo, FStaticMeshAsset& InOutAsset)
{
	InOutAsset.MaterialSlots.clear();
	InOutAsset.Sections.clear();

	FString CurrentMaterialName;
	FStaticMeshSection* CurrentSection = nullptr;
	uint32 RunningIndex = 0;
	TMap<FString, uint32> MaterialSlotLookup;

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
			Section.MaterialSlotIndex = FindOrAddMaterialSlot(
				Face.MaterialName,
				InObjInfo,
				InOutAsset,
				MaterialSlotLookup);

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
