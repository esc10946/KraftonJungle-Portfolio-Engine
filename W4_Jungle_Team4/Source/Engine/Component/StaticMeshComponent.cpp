#include "StaticMeshComponent.h"
DEFINE_CLASS(UStaticMeshComponent, UMeshComponent)
REGISTER_FACTORY(UStaticMeshComponent)

void UStaticMeshComponent::SetStaticMesh(UStaticMeshAsset* InMesh)
{
}

void UStaticMeshComponent::SetMaterial(int32 SlotIndex, UMaterial* InMaterial)
{
}

UMaterial* UStaticMeshComponent::GetMaterial(int32 SlotIndex) const
{
	return nullptr;
}

int32 UStaticMeshComponent::GetNumMaterials() const
{
	return int32();
}

void UStaticMeshComponent::UpdateWorldAABB() const
{
}

void UStaticMeshComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
}

void UStaticMeshComponent::PostEditProperty(const char* PropertyName)
{
}
