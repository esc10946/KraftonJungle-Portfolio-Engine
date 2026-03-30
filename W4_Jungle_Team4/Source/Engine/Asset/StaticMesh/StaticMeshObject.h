#pragma once

#include "Asset/Mesh/MeshObject.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"

class UMaterialInterface;

class UStaticMesh : public UMesh
{
public:
	DECLARE_CLASS(UStaticMesh, UMesh)

	UStaticMesh() = default;
	~UStaticMesh() override = default;

	void SetStaticMeshAsset(FStaticMeshAsset* InStaticMeshAsset);

	const FString& GetAssetPathFileName() const;
	FString GetDisplayName() const;
	bool HasGeometry() const { return !Vertices.empty() && !Indices.empty(); }
	const TArray<FStaticMeshVertex>& GetVertices() const { return Vertices; }
	const TArray<uint32>& GetIndices() const { return Indices; }
	const TArray<FStaticMeshSection>& GetSections() const { return Sections; }
	const TArray<FStaticMaterialSlot>& GetMaterialSlots() const;
	UMaterialInterface* GetDefaultMaterial(int32 SlotIndex) const;
	UMaterialInterface* GetDefaultMaterialForSection(int32 SectionIndex) const;
	bool SectionUsesEmbeddedAtlasUVs(int32 SectionIndex) const;
	int32 GetNumMaterialSlots() const override;
	int32 GetNumSections() const { return static_cast<int32>(Sections.size()); }
	const FVector& GetBoundsMin() const { return BoundsMin; }
	const FVector& GetBoundsMax() const { return BoundsMax; }
	bool BuildRenderMeshData(FMeshData& OutMeshData) const;
	bool BuildTexturedRenderMeshData(FTexturedMeshData& OutMeshData) const;

private:
	FString SourcePath;
	FString BakePath;
	TArray<FStaticMeshVertex> Vertices;
	TArray<uint32> Indices;
	TArray<FStaticMaterialSlot> MaterialSlots;
	TArray<FStaticMeshSection> Sections;
	FVector BoundsMin;
	FVector BoundsMax;
};
