#pragma once

#include "Core/CoreTypes.h"
#include "Core/Singleton.h"

class UMaterial;
struct FStaticMaterialSlot;

class FMaterialManager : public TSingleton<FMaterialManager>
{
	friend class TSingleton<FMaterialManager>;

public:
	UMaterial* GetOrCreateMaterial(const FStaticMaterialSlot& InSlot);
	UMaterial* FindMaterial(const FString& InKey) const;
	void PreloadMaterialResources(UMaterial* InMaterial);
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

private:
	FMaterialManager() = default;
	FString MakeMaterialKey(const FStaticMaterialSlot& InSlot) const;

private:
	TMap<FString, UMaterial*> MaterialMap;
};
