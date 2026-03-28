#include "Asset/Material/MaterialManager.h"

#include "Asset/Material/MaterialObject.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/Texture/TextureManager.h"
#include "Asset/Texture/TextureObject.h"
#include "Core/ResourceManager.h"
#include "Engine/Runtime/Engine.h"
#include "Object/FName.h"
#include "Object/Object.h"

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
