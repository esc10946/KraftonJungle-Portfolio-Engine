#pragma once
#include "PrimitiveComponent.h"

class UMaterial;

class UMeshComponent :
    public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UMeshComponent, UPrimitiveComponent)

	UMeshComponent() = default;
	virtual ~UMeshComponent() = default;

    virtual void       SetMaterial(int32 SlotIndex, UMaterial* InMaterial) = 0;
    virtual UMaterial* GetMaterial(int32 SlotIndex) const = 0;
    virtual int32      GetNumMaterials() const = 0;
	
	virtual void UpdateWorldAABB() const = 0;

	bool bUseUVScroll = false;
    FVector2 UVScrollSpeed = FVector2(0.f, 0.1f);
protected:
    TArray<UMaterial*> OverrideMaterials;
};

