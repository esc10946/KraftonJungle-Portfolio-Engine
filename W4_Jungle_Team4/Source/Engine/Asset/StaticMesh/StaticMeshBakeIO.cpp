#include "Asset/StaticMesh/StaticMeshBakeIO.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Serialization/Binary/WindowsBinReader.h"
#include "Serialization/Binary/WindowsBinWriter.h"

#include <filesystem>
#include <system_error>

namespace
{
	constexpr uint32 BakeMagic = 0x534D424B;   // 'SMBK'
	constexpr uint32 BakeVersion = 4;

	bool SerializeUInt32(FArchive& Ar, uint32& Value)
	{
		return Ar.Serialize(&Value, sizeof(Value));
	}

	bool SerializeFloat(FArchive& Ar, float& Value)
	{
		return Ar.Serialize(&Value, sizeof(Value));
	}

	bool SerializeString(FArchive& Ar, FString& Value)
	{
		if (Ar.IsSaving())
		{
			uint32 Length = static_cast<uint32>(Value.size());
			if (!SerializeUInt32(Ar, Length))
			{
				return false;
			}

			if (Length == 0)
			{
				return true;
			}

			return Ar.Serialize(Value.data(), Length);
		}

		uint32 Length = 0;
		if (!SerializeUInt32(Ar, Length))
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

	bool SerializeVector3(FArchive& Ar, FVector& Value)
	{
		return SerializeFloat(Ar, Value.X) &&
			SerializeFloat(Ar, Value.Y) &&
			SerializeFloat(Ar, Value.Z);
	}

	bool SerializeVector2(FArchive& Ar, FVector2& Value)
	{
		return SerializeFloat(Ar, Value.X) &&
			SerializeFloat(Ar, Value.Y);
	}

	bool SaveVertices(FArchive& Ar, const TArray<FStaticMeshVertex>& Vertices)
	{
		uint32 Count = static_cast<uint32>(Vertices.size());
		if (!SerializeUInt32(Ar, Count))
		{
			return false;
		}

		for (const FStaticMeshVertex& SrcVertex : Vertices)
		{
			FStaticMeshVertex Vertex = SrcVertex;
			if (!SerializeVector3(Ar, Vertex.Position) ||
				!SerializeVector3(Ar, Vertex.Normal) ||
				!SerializeVector2(Ar, Vertex.TexCoord))
			{
				return false;
			}
		}

		return true;
	}

	bool LoadVertices(FArchive& Ar, TArray<FStaticMeshVertex>& OutVertices)
	{
		uint32 Count = 0;
		if (!SerializeUInt32(Ar, Count))
		{
			return false;
		}

		OutVertices.clear();
		OutVertices.resize(Count);

		for (uint32 Index = 0; Index < Count; ++Index)
		{
			if (!SerializeVector3(Ar, OutVertices[Index].Position) ||
				!SerializeVector3(Ar, OutVertices[Index].Normal) ||
				!SerializeVector2(Ar, OutVertices[Index].TexCoord))
			{
				return false;
			}
		}

		return true;
	}

	bool SaveIndices(FArchive& Ar, const TArray<uint32>& Indices)
	{
		uint32 Count = static_cast<uint32>(Indices.size());
		if (!SerializeUInt32(Ar, Count))
		{
			return false;
		}

		for (uint32 Value : Indices)
		{
			if (!SerializeUInt32(Ar, Value))
			{
				return false;
			}
		}

		return true;
	}

	bool LoadIndices(FArchive& Ar, TArray<uint32>& OutIndices)
	{
		uint32 Count = 0;
		if (!SerializeUInt32(Ar, Count))
		{
			return false;
		}

		OutIndices.clear();
		OutIndices.resize(Count);

		for (uint32 Index = 0; Index < Count; ++Index)
		{
			if (!SerializeUInt32(Ar, OutIndices[Index]))
			{
				return false;
			}
		}

		return true;
	}

	bool SaveSections(FArchive& Ar, const TArray<FStaticMeshSection>& Sections)
	{
		uint32 Count = static_cast<uint32>(Sections.size());
		if (!SerializeUInt32(Ar, Count))
		{
			return false;
		}

		for (const FStaticMeshSection& SrcSection : Sections)
		{
			FStaticMeshSection Section = SrcSection;
			if (!SerializeUInt32(Ar, Section.FirstIndex) ||
				!SerializeUInt32(Ar, Section.IndexCount) ||
				!SerializeUInt32(Ar, Section.MaterialSlotIndex))
			{
				return false;
			}
		}

		return true;
	}

	bool LoadSections(FArchive& Ar, TArray<FStaticMeshSection>& OutSections)
	{
		uint32 Count = 0;
		if (!SerializeUInt32(Ar, Count))
		{
			return false;
		}

		OutSections.clear();
		OutSections.resize(Count);

		for (uint32 Index = 0; Index < Count; ++Index)
		{
			if (!SerializeUInt32(Ar, OutSections[Index].FirstIndex) ||
				!SerializeUInt32(Ar, OutSections[Index].IndexCount) ||
				!SerializeUInt32(Ar, OutSections[Index].MaterialSlotIndex))
			{
				return false;
			}
		}

		return true;
	}

	bool SaveMaterialSlots(FArchive& Ar, const TArray<FStaticMaterialSlot>& MaterialSlots)
	{
		uint32 Count = static_cast<uint32>(MaterialSlots.size());
		if (!SerializeUInt32(Ar, Count))
		{
			return false;
		}

		for (const FStaticMaterialSlot& SrcSlot : MaterialSlots)
		{
			FStaticMaterialSlot Slot = SrcSlot;
			if (!SerializeString(Ar, Slot.SlotName) ||
				!SerializeString(Ar, Slot.SourceMaterialName) ||
				!SerializeString(Ar, Slot.DiffuseTexturePath))
			{
				return false;
			}
		}

		return true;
	}

	bool LoadMaterialSlots(FArchive& Ar, TArray<FStaticMaterialSlot>& OutMaterialSlots)
	{
		uint32 Count = 0;
		if (!SerializeUInt32(Ar, Count))
		{
			return false;
		}

		OutMaterialSlots.clear();
		OutMaterialSlots.resize(Count);

		for (uint32 Index = 0; Index < Count; ++Index)
		{
			if (!SerializeString(Ar, OutMaterialSlots[Index].SlotName) ||
				!SerializeString(Ar, OutMaterialSlots[Index].SourceMaterialName) ||
				!SerializeString(Ar, OutMaterialSlots[Index].DiffuseTexturePath))
			{
				return false;
			}

			OutMaterialSlots[Index].DefaultMaterial = nullptr;
		}

		return true;
	}
}

FString FStaticMeshBakeIO::MakeBakePath(const FString& InSourcePath)
{
	if (InSourcePath.empty())
	{
		return FString();
	}

	std::filesystem::path Path(InSourcePath);
	Path.replace_extension(".smbin");
	return Path.lexically_normal().string();
}

bool FStaticMeshBakeIO::IsBakeUpToDate(const FString& InSourcePath, const FString& InBakePath)
{
	// bake >= obj : obj가 마지막으로 수정된 뒤에 bake가 만들어졌거나 다시 저장됨.
	// bake < obj : bake가 만들어진 뒤에 obj 수정됨 -> bake는 stale 상태.
	if (InSourcePath.empty() || InBakePath.empty())
	{
		return false;
	}

	std::error_code ErrorCode;

	const std::filesystem::path SourcePath(InSourcePath);
	const std::filesystem::path BakePath(InBakePath);

	if (!std::filesystem::exists(SourcePath, ErrorCode) ||
		!std::filesystem::exists(BakePath, ErrorCode))
	{
		return false;
	}

	const auto SourceTime = std::filesystem::last_write_time(SourcePath, ErrorCode);
	if (ErrorCode)
	{
		return false;
	}

	const auto BakeTime = std::filesystem::last_write_time(BakePath, ErrorCode);
	if (ErrorCode)
	{
		return false;
	}

	return BakeTime >= SourceTime;
}

bool FStaticMeshBakeIO::SaveToFile(const FString& InBakePath, const FStaticMeshAsset& InAsset)
{
	if (InBakePath.empty())
	{
		return false;
	}

	std::error_code ErrorCode;
	const std::filesystem::path BakePath(InBakePath);
	std::filesystem::create_directories(BakePath.parent_path(), ErrorCode);

	FWindowsBinWriter Writer(InBakePath.c_str());
	if (!Writer.IsValid())
	{
		Writer.Close();
		return false;
	}

	uint32 Magic = BakeMagic;
	uint32 Version = BakeVersion;
	FString SourcePath = InAsset.SourcePath;

	bool bSuccess =
		SerializeUInt32(Writer, Magic) &&
		SerializeUInt32(Writer, Version) &&
		SerializeString(Writer, SourcePath) &&
		SaveVertices(Writer, InAsset.Vertices) &&
		SaveIndices(Writer, InAsset.Indices) &&
		SaveMaterialSlots(Writer, InAsset.MaterialSlots) &&
		SaveSections(Writer, InAsset.Sections);

	Writer.Close();
	return bSuccess;
}

bool FStaticMeshBakeIO::LoadFromFile(const FString& InBakePath, FStaticMeshAsset& OutAsset)
{
	if (InBakePath.empty())
	{
		return false;
	}

	FWindowsBinReader Reader(InBakePath.c_str()); // 바이너리 파일 읽기 전용 reader 객체
	if (!Reader.IsValid())
	{
		Reader.Close();
		return false;
	}

	uint32 Magic = 0;
	uint32 Version = 0;

	bool bSuccess =
		SerializeUInt32(Reader, Magic) &&
		SerializeUInt32(Reader, Version);

	if (!bSuccess || Magic != BakeMagic || Version != BakeVersion)
	{
		Reader.Close();
		return false;
	}

	OutAsset.BakePath = InBakePath;
	OutAsset.Vertices.clear();
	OutAsset.Indices.clear();
	OutAsset.MaterialSlots.clear();
	OutAsset.Sections.clear();

	// 메타데이터를 OutAsset에 기록
	bSuccess =
		SerializeString(Reader, OutAsset.SourcePath) && // 원본 obj 경로 문자열 읽기
		LoadVertices(Reader, OutAsset.Vertices) &&
		LoadIndices(Reader, OutAsset.Indices) && 
		LoadMaterialSlots(Reader, OutAsset.MaterialSlots) && //  
		LoadSections(Reader, OutAsset.Sections);

	if (bSuccess)
	{
		OutAsset.UpdateBounds();
	}

	Reader.Close();
	return bSuccess;
}
