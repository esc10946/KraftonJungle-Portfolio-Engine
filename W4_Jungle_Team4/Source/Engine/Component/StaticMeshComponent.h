#pragma once
#include "MeshComponent.h"

class UStaticMeshAsset;

class UStaticMeshComponent :
    public UMeshComponent
{
	UStaticMeshAsset* StaticMeshAsset;
public:
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

    UStaticMeshComponent()          = default;
    ~UStaticMeshComponent() override = default;

    void SetStaticMesh(UStaticMeshAsset* InMesh);
 
    /** 현재 참조 중인 UStaticMesh 에셋을 반환한다. nullptr 가능. */
    UStaticMeshAsset* GetStaticMesh() const { return StaticMeshAsset; }
	
    virtual void       SetMaterial(int32 SlotIndex, UMaterial* InMaterial) override;
    virtual UMaterial* GetMaterial(int32 SlotIndex) const override;
    /** 에셋의 섹션 수(= 머티리얼 슬롯 수)를 반환한다. */
    virtual int32      GetNumMaterials() const override;

	
	virtual void UpdateWorldAABB() const override;
	
	void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    virtual void PostEditProperty(const char* PropertyName) override;

	virtual EPrimitiveType GetPrimitiveType() const override { return EPrimitiveType::EPT_StaticMesh; }
};

