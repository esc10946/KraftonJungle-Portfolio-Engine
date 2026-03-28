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
	~UStaticMeshComponent() override ;

	void SetStaticMesh(UStaticMesh* InMesh);
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }

	virtual void SetMaterial(int32 SlotIndex, UMaterial* InMaterial) override;
	virtual UMaterial* GetMaterial(int32 SlotIndex) const override;

	// 에셋의 섹션 수(= 머티리얼 슬롯 수)를 반환한다.
	virtual int32 GetNumMaterials() const override;

	virtual void UpdateWorldAABB() const override;

	void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
	virtual void PostEditProperty(const char* PropertyName) override;

	virtual EPrimitiveType GetPrimitiveType() const override { return EPrimitiveType::EPT_StaticMesh; }

private:
	
	//mesh의 vertices와 indices를 업데이트하는 함수
	//void RefreshMeshData();

private:
	UStaticMesh* StaticMesh;
};
