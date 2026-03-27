#pragma once

#include "MeshComponent.h"
#include "Render/Resource/VertexTypes.h"

class UMaterial;
class UStaticMesh;
struct FStaticMeshAsset;

class UStaticMeshComponent : public UMeshComponent
{
public:
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

	UStaticMeshComponent();
	~UStaticMeshComponent() override = default;

	void SetStaticMesh(UStaticMesh* InMesh);
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }
	FStaticMeshAsset* GetStaticMeshAsset() const;
	FString GetStaticMeshAssetPath() const;

	virtual void SetMaterial(int32 SlotIndex, UMaterial* InMaterial) override;
	virtual UMaterial* GetMaterial(int32 SlotIndex) const override;
	virtual int32 GetNumMaterials() const override;

	virtual void UpdateWorldAABB() const override;

	void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
	virtual void PostEditProperty(const char* PropertyName) override;

	virtual EPrimitiveType GetPrimitiveType() const override { return EPrimitiveType::EPT_StaticMesh; }

private:
	void RefreshMeshData();

private:
	UStaticMesh* StaticMesh = nullptr;
	FMeshData StaticMeshRenderData;
};
