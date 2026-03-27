#include "Asset/StaticMesh/StaticMeshBakeIO.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Serialization/Binary/Archive.h"
#include "Serialization/Binary/WindowsBinReader.h"
#include "Serialization/Binary/WindowsBinWriter.h"

#include <filesystem>

namespace
{
	template<typename TValue>
	bool SerializeValue(FArchive& Ar, TValue& Value)
	{
		return Ar.Serialize(&Value, sizeof(TValue));
	}

	bool SerializeString(FArchive& Ar, FString& Value)
	{
		uint32 Length = static_cast<uint32>(Value.size());
		if (Ar.IsSaving())
		{
			if (!SerializeValue(Ar, Length))
			{
				return false;
			}

			if (Length == 0)
			{
				return true;
			}

			return Ar.Serialize(Value.data(), Length);
		}

		if (!SerializeValue(Ar, Length))
		{
			return false;
		}

		Value.resize(Length);
		if (Length == 0)
		{
			return true;
		}

		return Ar.Serialize(Value.data(), Length);
	}
}

FString FStaticMeshBakeIO::MakeBakePath(const FString& InObjPath)
{
	std::filesystem::path SourcePath(InObjPath);
	const std::filesystem::path BakeDirectory = SourcePath.parent_path() / "Baked";
	return (BakeDirectory / SourcePath.stem()).replace_extension(".smesh").string();
}

bool FStaticMeshBakeIO::IsBakeUpToDate(const FString& InObjPath, const FString& InBakePath)
{
	namespace fs = std::filesystem;

	if (!fs::exists(InObjPath) || !fs::exists(InBakePath))
	{
		return false;
	}

	return fs::last_write_time(InBakePath) >= fs::last_write_time(InObjPath);
}

bool FStaticMeshBakeIO::SaveToFile(const FString& InBakePath, const FStaticMeshAsset& InAsset)
{
	std::filesystem::create_directories(std::filesystem::path(InBakePath).parent_path());
	FWindowsBinWriter Writer(InBakePath.c_str());
	if (!Writer.IsValid())
	{
		return false;
	}

	const bool bSaved = Save(Writer, InAsset);
	Writer.Close();
	return bSaved;
}

bool FStaticMeshBakeIO::LoadFromFile(const FString& InBakePath, FStaticMeshAsset& OutAsset)
{
	FWindowsBinReader Reader(InBakePath.c_str());
	if (!Reader.IsValid())
	{
		return false;
	}

	const bool bLoaded = Load(Reader, OutAsset);
	Reader.Close();
	return bLoaded;
}

bool FStaticMeshBakeIO::Save(FArchive& Ar, const FStaticMeshAsset& InAsset)
{
	if (!Ar.IsSaving() || !Ar.IsValid())
	{
		return false;
	}

	uint32 Version = 1;
	if (!SerializeValue(Ar, Version))
	{
		return false;
	}

	FString SourcePath = InAsset.SourcePath;
	FString BakePath = InAsset.BakePath;
	if (!SerializeString(Ar, SourcePath) || !SerializeString(Ar, BakePath))
	{
		return false;
	}

	uint32 VertexCount = static_cast<uint32>(InAsset.Vertices.size());
	uint32 IndexCount = static_cast<uint32>(InAsset.Indices.size());
	uint32 SectionCount = static_cast<uint32>(InAsset.Sections.size());
	if (!SerializeValue(Ar, VertexCount) || !SerializeValue(Ar, IndexCount) || !SerializeValue(Ar, SectionCount))
	{
		return false;
	}

	for (const FStaticMeshVertex& Vertex : InAsset.Vertices)
	{
		FStaticMeshVertex MutableVertex = Vertex;
		if (!SerializeValue(Ar, MutableVertex))
		{
			return false;
		}
	}

	for (uint32 Index : InAsset.Indices)
	{
		if (!SerializeValue(Ar, Index))
		{
			return false;
		}
	}

	for (const FStaticMeshSection& Section : InAsset.Sections)
	{
		FStaticMeshSection MutableSection = Section;
		if (!SerializeValue(Ar, MutableSection.FirstIndex) ||
			!SerializeValue(Ar, MutableSection.IndexCount) ||
			!SerializeString(Ar, MutableSection.MaterialName))
		{
			return false;
		}
	}

	FVector BoundsMin = InAsset.BoundsMin;
	FVector BoundsMax = InAsset.BoundsMax;
	return SerializeValue(Ar, BoundsMin) && SerializeValue(Ar, BoundsMax);
}

bool FStaticMeshBakeIO::Load(FArchive& Ar, FStaticMeshAsset& OutAsset)
{
	if (!Ar.IsLoading() || !Ar.IsValid())
	{
		return false;
	}

	uint32 Version = 0;
	if (!SerializeValue(Ar, Version) || Version != 1)
	{
		return false;
	}

	OutAsset.Reset();
	if (!SerializeString(Ar, OutAsset.SourcePath) || !SerializeString(Ar, OutAsset.BakePath))
	{
		return false;
	}

	uint32 VertexCount = 0;
	uint32 IndexCount = 0;
	uint32 SectionCount = 0;
	if (!SerializeValue(Ar, VertexCount) || !SerializeValue(Ar, IndexCount) || !SerializeValue(Ar, SectionCount))
	{
		return false;
	}

	OutAsset.Vertices.resize(VertexCount);
	for (FStaticMeshVertex& Vertex : OutAsset.Vertices)
	{
		if (!SerializeValue(Ar, Vertex))
		{
			return false;
		}
	}

	OutAsset.Indices.resize(IndexCount);
	for (uint32& Index : OutAsset.Indices)
	{
		if (!SerializeValue(Ar, Index))
		{
			return false;
		}
	}

	OutAsset.Sections.resize(SectionCount);
	for (FStaticMeshSection& Section : OutAsset.Sections)
	{
		if (!SerializeValue(Ar, Section.FirstIndex) ||
			!SerializeValue(Ar, Section.IndexCount) ||
			!SerializeString(Ar, Section.MaterialName))
		{
			return false;
		}
	}

	return SerializeValue(Ar, OutAsset.BoundsMin) && SerializeValue(Ar, OutAsset.BoundsMax);
}
