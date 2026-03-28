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

private:
	FMaterialManager() = default;
	FString MakeMaterialKey(const FStaticMaterialSlot& InSlot) const;

private:
	TMap<FString, UMaterial*> MaterialMap;
};
