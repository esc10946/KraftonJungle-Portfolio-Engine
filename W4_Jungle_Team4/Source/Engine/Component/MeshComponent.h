#pragma once
#include "PrimitiveComponent.h"

class UMaterialInterface;

class UMeshComponent :
    public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UMeshComponent, UPrimitiveComponent)

	UMeshComponent() = default;
	virtual ~UMeshComponent() = default;

    virtual void       SetMaterial(int32 SlotIndex, UMaterialInterface* InMaterial) = 0;
    virtual UMaterialInterface* GetMaterial(int32 SlotIndex) const = 0;
    virtual int32      GetNumMaterials() const = 0;
	
	virtual void UpdateWorldAABB() const = 0;
protected:
    TArray<UMaterialInterface*> OverrideMaterials;
};

