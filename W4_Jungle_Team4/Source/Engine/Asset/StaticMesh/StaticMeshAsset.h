#pragma once

#include "Core/CoreTypes.h"
#include "Math/Vector.h"
#include "Render/Resource/VertexTypes.h"

/**
@brief 최종 렌더링에 들어갈 정점 1개 데이터.
*/
struct FStaticMeshVertex
{
	FVector Position;
	FVector Normal;
	FVector2 TexCoord;
};

/**
@brief 메시를 머티리얼 단위로 나눈 구간 정보
*/
class UMaterial;

struct FStaticMaterialSlot
{
	FString SlotName;
	FString SourceMaterialName;
	FString DiffuseTexturePath;
	UMaterial* DefaultMaterial = nullptr;
};

struct FStaticMeshSection
{
	uint32 FirstIndex = 0;
	uint32 IndexCount = 0;
	uint32 MaterialSlotIndex = 0;
};


/**
 * @brief static mesh 전체를 담는 cooked 결과.
 * Vertices, Indices, Sections, 원본 경로 한 덩어리로 들고 있는 최종 asset
 */
struct FStaticMeshAsset
{
	FString SourcePath;
	FString BakePath;

	TArray<FStaticMeshVertex> Vertices;
	TArray<uint32> Indices;
	TArray<FStaticMaterialSlot> MaterialSlots;
	TArray<FStaticMeshSection> Sections;
	FVector BoundsMin;
	FVector BoundsMax;

	void UpdateBounds()
	{
		if (Vertices.empty())
		{
			BoundsMin = FVector();
			BoundsMax = FVector();
			return;
		}

		BoundsMin = Vertices[0].Position;
		BoundsMax = Vertices[0].Position;

		for (const FStaticMeshVertex& Vertex : Vertices)
		{
			BoundsMin.X = (BoundsMin.X < Vertex.Position.X) ? BoundsMin.X : Vertex.Position.X;
			BoundsMin.Y = (BoundsMin.Y < Vertex.Position.Y) ? BoundsMin.Y : Vertex.Position.Y;
			BoundsMin.Z = (BoundsMin.Z < Vertex.Position.Z) ? BoundsMin.Z : Vertex.Position.Z;

			BoundsMax.X = (BoundsMax.X > Vertex.Position.X) ? BoundsMax.X : Vertex.Position.X;
			BoundsMax.Y = (BoundsMax.Y > Vertex.Position.Y) ? BoundsMax.Y : Vertex.Position.Y;
			BoundsMax.Z = (BoundsMax.Z > Vertex.Position.Z) ? BoundsMax.Z : Vertex.Position.Z;
		}
	}

	bool BuildRenderMeshData(FMeshData& OutMeshData) const
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

	bool BuildTexturedRenderMeshData(FTexturedMeshData& OutMeshData) const
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
			RenderVertex.TexCoord = Vertex.TexCoord;
			OutMeshData.Vertices.push_back(RenderVertex);
		}

		return true;
	}
};
