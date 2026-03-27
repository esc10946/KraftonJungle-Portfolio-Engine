#include "Asset/StaticMesh/StaticMeshAsset.h"

#include "Render/Resource/VertexTypes.h"

#include <algorithm>

void FStaticMeshAsset::Reset()
{
	SourcePath.clear();
	BakePath.clear();
	Vertices.clear();
	Indices.clear();
	Sections.clear();
	BoundsMin = FVector();
	BoundsMax = FVector();
}

bool FStaticMeshAsset::IsValid() const
{
	return !Vertices.empty() && !Indices.empty();
}

bool FStaticMeshAsset::BuildRenderMeshData(FMeshData& OutMeshData) const
{
	OutMeshData.Vertices.clear();
	OutMeshData.Indices = Indices;

	if (!IsValid())
	{
		return false;
	}

	OutMeshData.Vertices.reserve(Vertices.size());
	for (const FStaticMeshVertex& Vertex : Vertices)
	{
		FVertex RenderVertex;
		RenderVertex.Position = Vertex.Position;
		RenderVertex.Color = Vertex.Color;
		RenderVertex.SubID = 0;
		OutMeshData.Vertices.push_back(RenderVertex);
	}

	return true;
}

void FStaticMeshAsset::UpdateBounds()
{
	if (Vertices.empty())
	{
		BoundsMin = FVector();
		BoundsMax = FVector();
		return;
	}

	BoundsMin = Vertices.front().Position;
	BoundsMax = Vertices.front().Position;

	for (const FStaticMeshVertex& Vertex : Vertices)
	{
		BoundsMin.X = std::min(BoundsMin.X, Vertex.Position.X);
		BoundsMin.Y = std::min(BoundsMin.Y, Vertex.Position.Y);
		BoundsMin.Z = std::min(BoundsMin.Z, Vertex.Position.Z);

		BoundsMax.X = std::max(BoundsMax.X, Vertex.Position.X);
		BoundsMax.Y = std::max(BoundsMax.Y, Vertex.Position.Y);
		BoundsMax.Z = std::max(BoundsMax.Z, Vertex.Position.Z);
	}
}
