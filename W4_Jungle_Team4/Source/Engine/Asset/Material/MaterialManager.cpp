#include "Asset/Material/MaterialManager.h"

#include "Asset/Material/MaterialObject.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/Texture/TextureManager.h"
#include "Asset/Texture/TextureObject.h"
#include "Core/ResourceManager.h"
#include "Engine/Runtime/Engine.h"
#include "Object/FName.h"
#include "Object/Object.h"
#include "Core/Paths.h"

UMaterial* FMaterialManager::GetOrCreateMaterial(const FStaticMaterialSlot& InSlot)
{
	const FString MaterialKey = MakeMaterialKey(InSlot);

	if (UMaterial* FoundMaterial = FindMaterial(MaterialKey))
	{
		return FoundMaterial;
	}

	UMaterial* NewMaterial = ConstructObject<UMaterial>();
	NewMaterial->SetSlotName(InSlot.SlotName);
	NewMaterial->SetSourceMaterialName(InSlot.SourceMaterialName);
	NewMaterial->SetDiffuseTexturePath(InSlot.DiffuseTexturePath);
	NewMaterial->SetDiffuseTexture(FTextureManager::Get().GetOrCreateTexture(InSlot.DiffuseTexturePath));

	if (!InSlot.SourceMaterialName.empty())
	{
		NewMaterial->SetFName(FName(InSlot.SourceMaterialName));
	}
	else if (!InSlot.SlotName.empty())
	{
		NewMaterial->SetFName(FName(InSlot.SlotName));
	}

	MaterialMap[MaterialKey] = NewMaterial;
	return NewMaterial;
}

UMaterial* FMaterialManager::FindMaterial(const FString& InKey) const
{
	const auto It = MaterialMap.find(InKey);
	return (It != MaterialMap.end()) ? It->second : nullptr;
}

void FMaterialManager::PreloadMaterialResources(UMaterial* InMaterial)
{
	if (!InMaterial)
	{
		return;
	}

	UTexture2D* Texture = InMaterial->GetDiffuseTexture();
	if (!Texture)
	{
		return;
	}

	const FString& TexturePath = Texture->GetSourcePath();
	if (TexturePath.empty())
	{
		return;
	}

	FResourceManager::Get().RegisterTexture(TexturePath);

	if (GEngine)
	{
		ID3D11Device* Device = GEngine->GetRenderer().GetFD3DDevice().GetDevice();
		FResourceManager::Get().GetOrCreateTextureSRV(TexturePath, Device);
	}
}

void FMaterialManager::Clear()
{
	MaterialMap.clear();
}

FString FMaterialManager::MakeMaterialKey(const FStaticMaterialSlot& InSlot) const
{
	return InSlot.SourceMaterialName + "|" + InSlot.DiffuseTexturePath;
}

UMaterial* FMaterialManager::RegisterAtlasMaterial(
	const FString& Name,
	const FString& TexturePath,
	int32 AtlasColumns,
	int32 AtlasRows,
	int32 TopTileX, int32 TopTileY,
	int32 SideTileX, int32 SideTileY,
	int32 BottomTileX, int32 BottomTileY)
{
	const FString Key = FString("Atlas|") + Name;

	if (UMaterial* FoundMaterial = FindMaterial(Key)) return FoundMaterial;

	UMaterial* NewMaterial = ConstructObject<UMaterial>();
	NewMaterial->SetFName(FName(Name));
	NewMaterial->SetSlotName(Name);
	NewMaterial->SetDiffuseTexturePath(TexturePath);
	NewMaterial->SetDiffuseTexture(FTextureManager::Get().GetOrCreateTexture(TexturePath));
	NewMaterial->SetUseAtlas(true);
	NewMaterial->SetAtlasLayout(AtlasColumns, AtlasRows);
	NewMaterial->SetTopTile(TopTileX, TopTileY);
	NewMaterial->SetSideTile(SideTileX, SideTileY);
	NewMaterial->SetBottomTile(BottomTileX, BottomTileY);

	MaterialMap[Key] = NewMaterial;
	PreloadMaterialResources(NewMaterial);
	return NewMaterial;
}

void FMaterialManager::RegisterDefaultBlockAtlasMaterials()
{
	const FString AtlasPath = FPaths::ToUtf8(FPaths::RootDir()) + "Data/Minecraft_All_block.png";

	UMaterial* Grass = RegisterAtlasMaterial("Atlas_Grass", AtlasPath, 1, 1, 0, 0, 0, 0, 0, 0);
	Grass->SetTopUVRect(FVector4(0.515625f, 0.242188f, 0.531250f, 0.257812f));
	Grass->SetSideUVRect(FVector4(0.421875f, 0.070312f, 0.437500f, 0.085938f));
	Grass->SetBottomUVRect(FVector4(0.406250f, 0.070312f, 0.421875f, 0.085938f));

	UMaterial* Dirt = RegisterAtlasMaterial("Atlas_Dirt", AtlasPath, 1, 1, 0, 0, 0, 0, 0, 0);
	const FVector4 DirtRect(0.406250f, 0.070312f, 0.421875f, 0.085938f);
	Dirt->SetTopUVRect(DirtRect);
	Dirt->SetSideUVRect(DirtRect);
	Dirt->SetBottomUVRect(DirtRect);

	UMaterial* TNT = RegisterAtlasMaterial("Atlas_TNT", AtlasPath, 1, 1, 0, 0, 0, 0, 0, 0);
	TNT->SetSideUVRect(FVector4(0.500000f, 0.070312f, 0.515625f, 0.085938f));
	TNT->SetTopUVRect(FVector4(0.515625f, 0.070312f, 0.531250f, 0.085938f));
	TNT->SetBottomUVRect(FVector4(0.531250f, 0.070312f, 0.546875f, 0.085938f));
}
