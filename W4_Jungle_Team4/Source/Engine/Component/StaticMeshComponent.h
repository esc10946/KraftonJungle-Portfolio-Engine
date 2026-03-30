#pragma once

#include "MeshComponent.h"
#include "Render/Resource/VertexTypes.h"

class UMaterialInterface;
class UStaticMesh;

class UStaticMeshComponent : public UMeshComponent
{
public:
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

	UStaticMeshComponent();
	~UStaticMeshComponent() override = default;

	void SetStaticMesh(UStaticMesh* InMesh);
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }
	FString GetStaticMeshAssetPath() const;
	const FTexturedMeshData* GetTexturedMeshData() const;
	uint32 GetTexturedMeshRevision() const { return TexturedMeshRevision; }

	void SetMaterial(int32 SlotIndex, UMaterialInterface* InMaterial) override;
	UMaterialInterface* GetMaterial(int32 SlotIndex) const override;
	int32 GetNumMaterials() const override;

	void UpdateWorldAABB() const override;

	void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
	void PostEditProperty(const char* PropertyName) override;

	EPrimitiveType GetPrimitiveType() const override { return EPrimitiveType::EPT_StaticMesh; }
	bool SupportsOutline() const override { return false; }

private:
	void InitializeMaterialSlotsFromMesh();
	void RefreshMeshData();

private:
	UStaticMesh* StaticMesh = nullptr;
	FMeshData StaticMeshRenderData;
	FTexturedMeshData StaticMeshTexturedRenderData;
	uint32 TexturedMeshRevision = 0;
	TArray<FString> EditableMaterialPropertyNames;
};
