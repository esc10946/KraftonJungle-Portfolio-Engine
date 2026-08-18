#include "Asset/Material/MaterialManager.h"

#include "Asset/Material/MaterialObject.h"
#include "Asset/Shader/ShaderManager.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/Texture/TextureManager.h"
#include "Asset/Texture/TextureObject.h"
#include "Core/ResourceManager.h"
#include "Engine/Runtime/Engine.h"
#include "Object/FName.h"
#include "Object/Object.h"
#include "Core/Paths.h"
#include <AssetPipeline/Obj/ObjTypes.h>

namespace
{
	void PreloadTextureResource(UTexture2D* InTexture)
	{
		if (!InTexture)
		{
			return;
		}

		const FString& TexturePath = InTexture->GetSourcePath();
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
}

FMaterialManager::~FMaterialManager()
{
	Clear();
}

UMaterial* FMaterialManager::CreateMaterial(const FObjMaterialInfo& InMaterialInfo)
{
    const FString MaterialKey = InMaterialInfo.Name + "|" + InMaterialInfo.map_Kd;

    if (UMaterialInterface* FoundMaterial = FindMaterial(MaterialKey))
    {
        return static_cast<UMaterial*>(FoundMaterial);
    }

    UMaterial* NewMaterial = ConstructObject<UMaterial>();
    NewMaterial->SetSlotName(InMaterialInfo.Name);
    NewMaterial->SetSourceMaterialName(InMaterialInfo.Name);
    NewMaterial->SetDiffuseTexturePath(InMaterialInfo.map_Kd);
    NewMaterial->SetDiffuseTexture(FTextureManager::Get().GetOrCreateTexture(InMaterialInfo.map_Kd));
    NewMaterial->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());
    NewMaterial->SetBaseColor(FVector4(InMaterialInfo.Kd, 1.0f));
    NewMaterial->SetFName(FName(InMaterialInfo.Name));

    MaterialMap[MaterialKey] = NewMaterial;
    return NewMaterial;
}


UMaterial* FMaterialManager::GetOrCreateMaterial(const FStaticMaterialSlot& InSlot)
{
	const FString MaterialKey = MakeMaterialKey(InSlot);

	if (UMaterialInterface* FoundMaterial = FindMaterial(MaterialKey))
	{
		return static_cast<UMaterial*>(FoundMaterial);
	}

	UMaterial* NewMaterial = ConstructObject<UMaterial>();
	NewMaterial->SetSlotName(InSlot.SlotName);
	NewMaterial->SetSourceMaterialName(InSlot.SourceMaterialName);
	NewMaterial->SetDiffuseTexturePath(InSlot.DiffuseTexturePath);
	NewMaterial->SetDiffuseTexture(FTextureManager::Get().GetOrCreateTexture(InSlot.DiffuseTexturePath));
	NewMaterial->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());

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

UMaterialInterface* FMaterialManager::FindMaterial(const FString& InKey) const
{
	const auto It = MaterialMap.find(InKey);
	return (It != MaterialMap.end()) ? It->second : nullptr;
}

UMaterialInstance* FMaterialManager::CreateMaterialInstance(UMaterial* ParentMaterial, const FString& InstanceName)
{
	if (!ParentMaterial)
	{
		return nullptr;
	}

	const FString Key = FString("MaterialInstance|") + InstanceName;
	if (!InstanceName.empty())
	{
		if (UMaterialInterface* FoundMaterial = FindMaterial(Key))
		{
			return FoundMaterial->IsA<UMaterialInstance>() ? static_cast<UMaterialInstance*>(FoundMaterial) : nullptr;
		}
	}

	UMaterialInstance* NewInstance = ConstructObject<UMaterialInstance>();
	NewInstance->SetParentMaterial(ParentMaterial);
	NewInstance->SetSlotName(ParentMaterial->GetSlotName());
	NewInstance->SetSourceMaterialName(ParentMaterial->GetSourceMaterialName());

	if (!InstanceName.empty())
	{
		NewInstance->SetFName(FName(InstanceName));
		MaterialMap[Key] = NewInstance;
	}

	return NewInstance;
}

void FMaterialManager::PreloadMaterialResources(UMaterialInterface* InMaterial)
{
	if (!InMaterial)
	{
		return;
	}

	FResolvedMaterialParameters Parameters;
	InMaterial->ResolveParameters(Parameters);
	PreloadTextureResource(Parameters.DiffuseTexture);
	PreloadTextureResource(Parameters.NormalTexture);
}

void FMaterialManager::Clear()
{
	for (auto& Pair : MaterialMap)
	{
		delete Pair.second; // 임의로 Material도 매니저에서 해제하도록 관리
	}

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

	if (UMaterialInterface* FoundMaterial = FindMaterial(Key)) return static_cast<UMaterial*>(FoundMaterial);

	UMaterial* NewMaterial = ConstructObject<UMaterial>();
	NewMaterial->SetFName(FName(Name));
	NewMaterial->SetSlotName(Name);
	NewMaterial->SetDiffuseTexturePath(TexturePath);
	NewMaterial->SetDiffuseTexture(FTextureManager::Get().GetOrCreateTexture(TexturePath));
	NewMaterial->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());
	NewMaterial->SetUseAtlas(true);
	NewMaterial->SetAtlasLayout(AtlasColumns, AtlasRows);
	NewMaterial->SetTopTile(TopTileX, TopTileY);
	NewMaterial->SetSideTile(SideTileX, SideTileY);
	NewMaterial->SetBottomTile(BottomTileX, BottomTileY);

	MaterialMap[Key] = NewMaterial;
	PreloadMaterialResources(NewMaterial);
	return NewMaterial;
}