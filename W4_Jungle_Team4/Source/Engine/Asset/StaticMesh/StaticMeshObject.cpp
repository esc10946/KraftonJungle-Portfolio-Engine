#include "Asset/StaticMesh/StaticMeshObject.h"

#include "Asset/Material/MaterialManager.h"
#include "Asset/Material/MaterialObject.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"

#include <filesystem>

DEFINE_CLASS(UStaticMesh, UMesh)
REGISTER_FACTORY(UStaticMesh)

void UStaticMesh::SetStaticMeshAsset(FStaticMeshAsset* InStaticMeshAsset)
{
	SourcePath.clear();
	BakePath.clear();
	Vertices.clear();
	Indices.clear();
	MaterialSlots.clear();
	Sections.clear();
	BoundsMin = FVector();
	BoundsMax = FVector();

	if (!InStaticMeshAsset)
	{
		return;
	}

	SourcePath = InStaticMeshAsset->SourcePath;
	BakePath = InStaticMeshAsset->BakePath;
	Vertices = InStaticMeshAsset->Vertices;
	Indices = InStaticMeshAsset->Indices;
	MaterialSlots = InStaticMeshAsset->MaterialSlots;
	Sections = InStaticMeshAsset->Sections;
	BoundsMin = InStaticMeshAsset->BoundsMin;
	BoundsMax = InStaticMeshAsset->BoundsMax;

	for (size_t SlotIndex = 0; SlotIndex < MaterialSlots.size(); ++SlotIndex)
	{
		UMaterialInterface* Material = FMaterialManager::Get().GetOrCreateMaterial(MaterialSlots[SlotIndex]);
		MaterialSlots[SlotIndex].DefaultMaterial = Material;
		FMaterialManager::Get().PreloadMaterialResources(Material);
	}
}

const FString& UStaticMesh::GetAssetPathFileName() const
{
	return SourcePath;
}

FString UStaticMesh::GetDisplayName() const
{
	if (SourcePath.empty())
	{
		return GetFName().ToString();
	}

	return std::filesystem::path(SourcePath).filename().string();
}

const TArray<FStaticMaterialSlot>& UStaticMesh::GetMaterialSlots() const
{
	return MaterialSlots;
}

UMaterialInterface* UStaticMesh::GetDefaultMaterial(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= static_cast<int32>(MaterialSlots.size()))
	{
		return nullptr;
	}

	return MaterialSlots[SlotIndex].DefaultMaterial;
}

UMaterialInterface* UStaticMesh::GetDefaultMaterialForSection(int32 SectionIndex) const
{
	if (SectionIndex < 0 || SectionIndex >= static_cast<int32>(Sections.size()))
	{
		return nullptr;
	}

	return GetDefaultMaterial(static_cast<int32>(Sections[SectionIndex].MaterialSlotIndex));
}

bool UStaticMesh::SectionUsesEmbeddedAtlasUVs(int32 SectionIndex) const
{
	if (SectionIndex < 0 || SectionIndex >= static_cast<int32>(Sections.size()) || Indices.empty() || Vertices.empty())
	{
		return false;
	}

	const FStaticMeshSection& Section = Sections[SectionIndex];
	if (Section.IndexCount == 0)
	{
		return false;
	}

	float MinU = 1.0f;
	float MinV = 1.0f;
	float MaxU = 0.0f;
	float MaxV = 0.0f;
	bool bHasValidVertex = false;

	const size_t SectionEndIndex = static_cast<size_t>(Section.FirstIndex + Section.IndexCount);
	const size_t EndIndex = (SectionEndIndex < Indices.size()) ? SectionEndIndex : Indices.size();
	for (size_t IndexPos = Section.FirstIndex; IndexPos < EndIndex; ++IndexPos)
	{
		const uint32 VertexIndex = Indices[IndexPos];
		if (VertexIndex >= Vertices.size())
		{
			continue;
		}

		const FVector2& UV = Vertices[VertexIndex].TexCoord;
		MinU = (UV.X < MinU) ? UV.X : MinU;
		MinV = (UV.Y < MinV) ? UV.Y : MinV;
		MaxU = (UV.X > MaxU) ? UV.X : MaxU;
		MaxV = (UV.Y > MaxV) ? UV.Y : MaxV;
		bHasValidVertex = true;
	}

	if (!bHasValidVertex)
	{
		return false;
	}

	const bool bLooksLikeFullFaceUV =
		MinU <= 0.001f && MinV <= 0.001f &&
		MaxU >= 0.999f && MaxV >= 0.999f;

	return !bLooksLikeFullFaceUV;
}

int32 UStaticMesh::GetNumMaterialSlots() const
{
	return static_cast<int32>(MaterialSlots.size());
}

bool UStaticMesh::BuildRenderMeshData(FMeshData& OutMeshData) const
{
	OutMeshData.Vertices.clear();
	OutMeshData.Indices.clear();

	if (Vertices.empty() || Indices.empty())
	{
		return false;
	}

	OutMeshData.Vertices.reserve(Vertices.size());
	OutMeshData.Indices = Indices;

	for (const FStaticMeshVertex& Vertex : Vertices)
	{
		FVertex RenderVertex;
		RenderVertex.Position = Vertex.Position;
		RenderVertex.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
		RenderVertex.SubID = 0;
		OutMeshData.Vertices.push_back(RenderVertex);
	}

	for (size_t SectionIndex = 0; SectionIndex < Sections.size(); ++SectionIndex)
	{
		const FStaticMeshSection& Section = Sections[SectionIndex];
		const size_t SectionEndIndex = static_cast<size_t>(Section.FirstIndex + Section.IndexCount);
		const size_t EndIndex = (SectionEndIndex < OutMeshData.Indices.size())
			? SectionEndIndex
			: OutMeshData.Indices.size();

		for (size_t IndexPos = Section.FirstIndex; IndexPos < EndIndex; ++IndexPos)
		{
			const uint32 VertexIndex = OutMeshData.Indices[IndexPos];
			if (VertexIndex < OutMeshData.Vertices.size())
			{
				OutMeshData.Vertices[VertexIndex].SubID = static_cast<int>(SectionIndex);
			}
		}
	}

	return true;
}

bool UStaticMesh::BuildTexturedRenderMeshData(FTexturedMeshData& OutMeshData) const
{
	OutMeshData.Vertices.clear();
	OutMeshData.Indices.clear();

	if (Vertices.empty() || Indices.empty())
	{
		return false;
	}

	OutMeshData.Vertices.reserve(Vertices.size());
	OutMeshData.Indices = Indices;

	for (const FStaticMeshVertex& Vertex : Vertices)
	{
		FTextureVertex RenderVertex;
		RenderVertex.Position = Vertex.Position;
		RenderVertex.Normal = Vertex.Normal;
		RenderVertex.TexCoord = Vertex.TexCoord;
		OutMeshData.Vertices.push_back(RenderVertex);
	}

	return true;
}
