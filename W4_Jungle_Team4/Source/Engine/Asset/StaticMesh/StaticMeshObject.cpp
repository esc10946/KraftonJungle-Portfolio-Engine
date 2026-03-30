#include "Asset/StaticMesh/StaticMeshObject.h"

#include "Asset/Material/MaterialManager.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"

#include <filesystem>

DEFINE_CLASS(UStaticMesh, UObject)
REGISTER_FACTORY(UStaticMesh)

void UStaticMesh::SetStaticMeshAsset(FStaticMeshAsset* InStaticMeshAsset)
{
	StaticMeshAsset = InStaticMeshAsset;
	MaterialSlots = StaticMeshAsset ? StaticMeshAsset->MaterialSlots : TArray<FStaticMaterialSlot>();

	for (FStaticMaterialSlot& Slot : MaterialSlots)
	{
		if (!Slot.DefaultMaterial)
		{
			Slot.DefaultMaterial = FMaterialManager::Get().GetOrCreateMaterial(Slot);
		}

		FMaterialManager::Get().PreloadMaterialResources(Slot.DefaultMaterial);
	}
}

const FString& UStaticMesh::GetAssetPathFileName() const
{
	static const FString EmptyPath;
	return StaticMeshAsset ? StaticMeshAsset->SourcePath : EmptyPath;
}

FString UStaticMesh::GetDisplayName() const
{
	const FString& AssetPath = GetAssetPathFileName();
	if (AssetPath.empty())
	{
		return GetFName().ToString();
	}

	return std::filesystem::path(AssetPath).filename().string();
}

UMaterial* UStaticMesh::GetDefaultMaterial(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= static_cast<int32>(MaterialSlots.size()))
	{
		return nullptr;
	}

	return MaterialSlots[SlotIndex].DefaultMaterial;
}

void UStaticMesh::SetDefaultMaterial(int32 SlotIndex, UMaterial* InMaterial)
{
	if (SlotIndex < 0)
	{
		return;
	}

	if (SlotIndex >= static_cast<int32>(MaterialSlots.size()))
	{
		MaterialSlots.resize(SlotIndex + 1);
	}

	MaterialSlots[SlotIndex].DefaultMaterial = InMaterial;
}
