#pragma once

#include "Core/CoreTypes.h"
#include "Core/Singleton.h"

class UMaterial;
class UMaterialInterface;
class UMaterialInstance;
struct FStaticMaterialSlot;

class FMaterialManager : public TSingleton<FMaterialManager>
{
	friend class TSingleton<FMaterialManager>;

public:
	UMaterial* GetOrCreateMaterial(const FStaticMaterialSlot& InSlot);
	UMaterialInterface* FindMaterial(const FString& InKey) const;
	UMaterialInstance* CreateMaterialInstance(UMaterial* ParentMaterial, const FString& InstanceName);
	void PreloadMaterialResources(UMaterialInterface* InMaterial);
	void Clear();

	UMaterial* RegisterAtlasMaterial(
		const FString& Name,
		const FString& TexturePath,
		int32 AtlasColumns,
		int32 AtlasRows,
		int32 TopTileX, int32 TopTileY,
		int32 SideTileX, int32 SideTileY,
		int32 BottomTileX, int32 BottomTileY);
	void RegisterDefaultBlockAtlasMaterials();
	void RegisterDefaultPreviewMaterials();

private:
	FMaterialManager() = default;
	FString MakeMaterialKey(const FStaticMaterialSlot& InSlot) const;

private:
	TMap<FString, UMaterialInterface*> MaterialMap;
};
