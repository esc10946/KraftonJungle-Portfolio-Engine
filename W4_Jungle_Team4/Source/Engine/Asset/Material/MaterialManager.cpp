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

void FMaterialManager::RegisterDefaultBlockAtlasMaterials()
{
	const FString AtlasPath = FPaths::ToUtf8(FPaths::TexturePath()) + "Minecraft_All_block.png";

	UMaterial* AtlasMaster = RegisterAtlasMaterial("Atlas_BlockMaster", AtlasPath, 1, 1, 0, 0, 0, 0, 0, 0);

	UMaterialInstance* Grass = CreateMaterialInstance(AtlasMaster, "Atlas_Grass");
	Grass->SetTopUVRectParameter(FVector4(0.515625f, 0.242188f, 0.531250f, 0.257812f));
	Grass->SetSideUVRectParameter(FVector4(0.421875f, 0.070312f, 0.437500f, 0.085938f));
	Grass->SetBottomUVRectParameter(FVector4(0.406250f, 0.070312f, 0.421875f, 0.085938f));
	PreloadMaterialResources(Grass);

	UMaterialInstance* Dirt = CreateMaterialInstance(AtlasMaster, "Atlas_Dirt");
	const FVector4 DirtRect(0.406250f, 0.070312f, 0.421875f, 0.085938f);
	Dirt->SetTopUVRectParameter(DirtRect);
	Dirt->SetSideUVRectParameter(DirtRect);
	Dirt->SetBottomUVRectParameter(DirtRect);
	PreloadMaterialResources(Dirt);

	UMaterialInstance* TNT = CreateMaterialInstance(AtlasMaster, "Atlas_TNT");
	TNT->SetSideUVRectParameter(FVector4(0.500000f, 0.070312f, 0.515625f, 0.085938f));
	TNT->SetTopUVRectParameter(FVector4(0.515625f, 0.070312f, 0.531250f, 0.085938f));
	TNT->SetBottomUVRectParameter(FVector4(0.531250f, 0.070312f, 0.546875f, 0.085938f));
	PreloadMaterialResources(TNT);
}

void FMaterialManager::RegisterDefaultPreviewMaterials()
{
	const FString CheckerPath = FPaths::ToUtf8(FPaths::TexturePath()) + "grass.jpg";
	const FString CubePath = FPaths::ToUtf8(FPaths::TexturePath()) + "cube-tex.png";
	const FString AtlasPath = FPaths::ToUtf8(FPaths::TexturePath()) + "Minecraft_All_block.png";

	auto RegisterSimpleMaterial = [this](const FString& Key, const FString& Name, const FString& TexturePath)
	{
		if (FindMaterial(Key))
		{
			return;
		}

		UMaterial* NewMaterial = ConstructObject<UMaterial>();
		NewMaterial->SetFName(FName(Name));
		NewMaterial->SetSlotName(Name);
		NewMaterial->SetSourceMaterialName(Name);
		NewMaterial->SetDiffuseTexturePath(TexturePath);
		NewMaterial->SetDiffuseTexture(FTextureManager::Get().GetOrCreateTexture(TexturePath));
		NewMaterial->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());
		NewMaterial->SetMetallic(0.0f);
		MaterialMap[Key] = NewMaterial;
		PreloadMaterialResources(NewMaterial);
	};

	RegisterSimpleMaterial("Preview|Checker", "Preview_Checker", CheckerPath);
	RegisterSimpleMaterial("Preview|CubeTex", "Preview_CubeTex", CubePath);
	RegisterSimpleMaterial("Preview|AtlasFull", "Preview_AtlasFull", AtlasPath);

	if (!FindMaterial("Preview|PaintedGreen"))
	{
		UMaterial* PaintedGreen = ConstructObject<UMaterial>();
		PaintedGreen->SetFName(FName("Preview_PaintedGreen"));
		PaintedGreen->SetSlotName("Preview_PaintedGreen");
		PaintedGreen->SetSourceMaterialName("Preview_PaintedGreen");
		PaintedGreen->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());
		PaintedGreen->SetBaseColor(FVector4(0.34f, 0.58f, 0.36f, 1.0f));
		PaintedGreen->SetMetallic(0.0f);
		MaterialMap["Preview|PaintedGreen"] = PaintedGreen;
	}

	if (!FindMaterial("Preview|RustBrown"))
	{
		UMaterial* RustBrown = ConstructObject<UMaterial>();
		RustBrown->SetFName(FName("Preview_RustBrown"));
		RustBrown->SetSlotName("Preview_RustBrown");
		RustBrown->SetSourceMaterialName("Preview_RustBrown");
		RustBrown->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());
		RustBrown->SetBaseColor(FVector4(0.55f, 0.38f, 0.22f, 1.0f));
		RustBrown->SetMetallic(0.0f);
		MaterialMap["Preview|RustBrown"] = RustBrown;
	}

	if (!FindMaterial("Preview|SemiGlossWhite"))
	{
		UMaterial* SemiGlossWhite = ConstructObject<UMaterial>();
		SemiGlossWhite->SetFName(FName("Preview_SemiGlossWhite"));
		SemiGlossWhite->SetSlotName("Preview_SemiGlossWhite");
		SemiGlossWhite->SetSourceMaterialName("Preview_SemiGlossWhite");
		SemiGlossWhite->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());
		SemiGlossWhite->SetBaseColor(FVector4(0.92f, 0.92f, 0.90f, 1.0f));
		SemiGlossWhite->SetMetallic(0.0f);
		MaterialMap["Preview|SemiGlossWhite"] = SemiGlossWhite;
	}

	if (!FindMaterial("Preview|CheckerMetal"))
	{
		UMaterial* MetallicChecker = ConstructObject<UMaterial>();
		MetallicChecker->SetFName(FName("Preview_CheckerMetal"));
		MetallicChecker->SetSlotName("Preview_CheckerMetal");
		MetallicChecker->SetSourceMaterialName("Preview_CheckerMetal");
		MetallicChecker->SetDiffuseTexturePath(CheckerPath);
		MetallicChecker->SetDiffuseTexture(FTextureManager::Get().GetOrCreateTexture(CheckerPath));
		MetallicChecker->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());
		MetallicChecker->SetMetallic(1.0f);
		MaterialMap["Preview|CheckerMetal"] = MetallicChecker;
		PreloadMaterialResources(MetallicChecker);
	}

	if (!FindMaterial("Preview|CheckerRough"))
	{
		UMaterial* RoughChecker = ConstructObject<UMaterial>();
		RoughChecker->SetFName(FName("Preview_CheckerRough"));
		RoughChecker->SetSlotName("Preview_CheckerRough");
		RoughChecker->SetSourceMaterialName("Preview_CheckerRough");
		RoughChecker->SetDiffuseTexturePath(CheckerPath);
		RoughChecker->SetDiffuseTexture(FTextureManager::Get().GetOrCreateTexture(CheckerPath));
		RoughChecker->SetShader(FShaderManager::Get().GetOrCreateStaticMeshShader());
		RoughChecker->SetMetallic(0.0f);
		MaterialMap["Preview|CheckerRough"] = RoughChecker;
		PreloadMaterialResources(RoughChecker);
	}
}
