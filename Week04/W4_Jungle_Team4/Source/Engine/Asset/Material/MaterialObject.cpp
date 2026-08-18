#include "Asset/Material/MaterialObject.h"

#include "Asset/Shader/ShaderObject.h"
#include "Asset/Texture/TextureObject.h"

namespace
{
	const FString& GetEmptyString()
	{
		static const FString Empty;
		return Empty;
	}

	const FVector4& GetDefaultColor()
	{
		static const FVector4 DefaultColor(1.0f, 1.0f, 1.0f, 1.0f);
		return DefaultColor;
	}

	const FVector4& GetDefaultUVRect()
	{
		static const FVector4 DefaultRect(0.0f, 0.0f, 1.0f, 1.0f);
		return DefaultRect;
	}
}

DEFINE_CLASS(UMaterialInterface, UObject)

DEFINE_CLASS(UMaterial, UMaterialInterface)
REGISTER_FACTORY(UMaterial)

DEFINE_CLASS(UMaterialInstance, UMaterialInterface)
REGISTER_FACTORY(UMaterialInstance)

void UMaterial::ResolveParameters(FResolvedMaterialParameters& OutParameters) const
{
	OutParameters = FResolvedMaterialParameters{};
	OutParameters.Shader = Shader;
	OutParameters.DiffuseTexture = DiffuseTexture;
	OutParameters.NormalTexture = NormalTexture;
	OutParameters.BaseColor = BaseColor;
	OutParameters.Metallic = Metallic;
	OutParameters.bUseAtlas = bUseAtlas;
	OutParameters.AtlasColumns = AtlasColumns;
	OutParameters.AtlasRows = AtlasRows;
	OutParameters.TopTileX = TopTileX;
	OutParameters.TopTileY = TopTileY;
	OutParameters.SideTileX = SideTileX;
	OutParameters.SideTileY = SideTileY;
	OutParameters.BottomTileX = BottomTileX;
	OutParameters.BottomTileY = BottomTileY;
	OutParameters.TopUVRect = TopUVRect;
	OutParameters.SideUVRect = SideUVRect;
	OutParameters.BottomUVRect = BottomUVRect;
}

void UMaterialInstance::SetTextureParameter(UTexture2D* InTexture)
{
	DiffuseTexture = InTexture;
	DiffuseTexturePath = InTexture ? InTexture->GetSourcePath() : FString();
	bOverrideDiffuseTexture = true;
}

void UMaterialInstance::ClearTextureParameter()
{
	DiffuseTexture = nullptr;
	DiffuseTexturePath.clear();
	bOverrideDiffuseTexture = false;
}

void UMaterialInstance::SetNormalTextureParameter(UTexture2D* InTexture)
{
	NormalTexture = InTexture;
	NormalTexturePath = InTexture ? InTexture->GetSourcePath() : FString();
	bOverrideNormalTexture = true;
}

void UMaterialInstance::ClearNormalTextureParameter()
{
	NormalTexture = nullptr;
	NormalTexturePath.clear();
	bOverrideNormalTexture = false;
}

void UMaterialInstance::SetShaderParameter(UShader* InShader)
{
	Shader = InShader;
	bOverrideShader = true;
}

void UMaterialInstance::ClearShaderParameter()
{
	Shader = nullptr;
	bOverrideShader = false;
}

void UMaterialInstance::SetBaseColorParameter(const FVector4& InColor)
{
	BaseColor = InColor;
	bOverrideBaseColor = true;
}

void UMaterialInstance::ClearBaseColorParameter()
{
	BaseColor = GetDefaultColor();
	bOverrideBaseColor = false;
}

void UMaterialInstance::SetMetallicParameter(float InMetallic)
{
	Metallic = InMetallic;
	bOverrideMetallic = true;
}

void UMaterialInstance::ClearMetallicParameter()
{
	Metallic = 0.0f;
	bOverrideMetallic = false;
}

void UMaterialInstance::SetTopUVRectParameter(const FVector4& InRect)
{
	TopUVRect = InRect;
	bOverrideTopUVRect = true;
}

void UMaterialInstance::SetSideUVRectParameter(const FVector4& InRect)
{
	SideUVRect = InRect;
	bOverrideSideUVRect = true;
}

void UMaterialInstance::SetBottomUVRectParameter(const FVector4& InRect)
{
	BottomUVRect = InRect;
	bOverrideBottomUVRect = true;
}

void UMaterialInstance::ClearUVRectParameters()
{
	TopUVRect = GetDefaultUVRect();
	SideUVRect = GetDefaultUVRect();
	BottomUVRect = GetDefaultUVRect();
	bOverrideTopUVRect = false;
	bOverrideSideUVRect = false;
	bOverrideBottomUVRect = false;
}

const FString& UMaterialInstance::GetSlotName() const
{
	if (!SlotName.empty())
	{
		return SlotName;
	}

	return ParentMaterial ? ParentMaterial->GetSlotName() : UMaterialInterface::GetSlotName();
}

const FString& UMaterialInstance::GetSourceMaterialName() const
{
	if (!SourceMaterialName.empty())
	{
		return SourceMaterialName;
	}

	return ParentMaterial ? ParentMaterial->GetSourceMaterialName() : UMaterialInterface::GetSourceMaterialName();
}

const FString& UMaterialInstance::GetDiffuseTexturePath() const
{
	if (bOverrideDiffuseTexture)
	{
		return DiffuseTexturePath;
	}

	return ParentMaterial ? ParentMaterial->GetDiffuseTexturePath() : GetEmptyString();
}

UTexture2D* UMaterialInstance::GetDiffuseTexture() const
{
	if (bOverrideDiffuseTexture)
	{
		return DiffuseTexture;
	}

	return ParentMaterial ? ParentMaterial->GetDiffuseTexture() : nullptr;
}

const FString& UMaterialInstance::GetNormalTexturePath() const
{
	if (bOverrideNormalTexture)
	{
		return NormalTexturePath;
	}

	return ParentMaterial ? ParentMaterial->GetNormalTexturePath() : GetEmptyString();
}

UTexture2D* UMaterialInstance::GetNormalTexture() const
{
	if (bOverrideNormalTexture)
	{
		return NormalTexture;
	}

	return ParentMaterial ? ParentMaterial->GetNormalTexture() : nullptr;
}

UShader* UMaterialInstance::GetShader() const
{
	if (bOverrideShader)
	{
		return Shader;
	}

	return ParentMaterial ? ParentMaterial->GetShader() : nullptr;
}

const FVector4& UMaterialInstance::GetBaseColor() const
{
	if (bOverrideBaseColor)
	{
		return BaseColor;
	}

	return ParentMaterial ? ParentMaterial->GetBaseColor() : GetDefaultColor();
}

float UMaterialInstance::GetMetallic() const
{
	if (bOverrideMetallic)
	{
		return Metallic;
	}

	return ParentMaterial ? ParentMaterial->GetMetallic() : 0.0f;
}

bool UMaterialInstance::UsesAtlas() const
{
	return ParentMaterial ? ParentMaterial->UsesAtlas() : false;
}

int32 UMaterialInstance::GetAtlasColumns() const
{
	return ParentMaterial ? ParentMaterial->GetAtlasColumns() : 1;
}

int32 UMaterialInstance::GetAtlasRows() const
{
	return ParentMaterial ? ParentMaterial->GetAtlasRows() : 1;
}

int32 UMaterialInstance::GetTopTileX() const
{
	return ParentMaterial ? ParentMaterial->GetTopTileX() : 0;
}

int32 UMaterialInstance::GetTopTileY() const
{
	return ParentMaterial ? ParentMaterial->GetTopTileY() : 0;
}

int32 UMaterialInstance::GetSideTileX() const
{
	return ParentMaterial ? ParentMaterial->GetSideTileX() : 0;
}

int32 UMaterialInstance::GetSideTileY() const
{
	return ParentMaterial ? ParentMaterial->GetSideTileY() : 0;
}

int32 UMaterialInstance::GetBottomTileX() const
{
	return ParentMaterial ? ParentMaterial->GetBottomTileX() : 0;
}

int32 UMaterialInstance::GetBottomTileY() const
{
	return ParentMaterial ? ParentMaterial->GetBottomTileY() : 0;
}

const FVector4& UMaterialInstance::GetTopUVRect() const
{
	if (bOverrideTopUVRect)
	{
		return TopUVRect;
	}

	return ParentMaterial ? ParentMaterial->GetTopUVRect() : GetDefaultUVRect();
}

const FVector4& UMaterialInstance::GetSideUVRect() const
{
	if (bOverrideSideUVRect)
	{
		return SideUVRect;
	}

	return ParentMaterial ? ParentMaterial->GetSideUVRect() : GetDefaultUVRect();
}

const FVector4& UMaterialInstance::GetBottomUVRect() const
{
	if (bOverrideBottomUVRect)
	{
		return BottomUVRect;
	}

	return ParentMaterial ? ParentMaterial->GetBottomUVRect() : GetDefaultUVRect();
}

void UMaterialInstance::ResolveParameters(FResolvedMaterialParameters& OutParameters) const
{
	if (ParentMaterial)
	{
		ParentMaterial->ResolveParameters(OutParameters);
	}
	else
	{
		OutParameters = FResolvedMaterialParameters{};
	}

	if (bOverrideShader)
	{
		OutParameters.Shader = Shader;
	}

	if (bOverrideDiffuseTexture)
	{
		OutParameters.DiffuseTexture = DiffuseTexture;
	}

	if (bOverrideNormalTexture)
	{
		OutParameters.NormalTexture = NormalTexture;
	}

	if (bOverrideBaseColor)
	{
		OutParameters.BaseColor = BaseColor;
	}

	if (bOverrideMetallic)
	{
		OutParameters.Metallic = Metallic;
	}

	if (bOverrideTopUVRect)
	{
		OutParameters.TopUVRect = TopUVRect;
	}

	if (bOverrideSideUVRect)
	{
		OutParameters.SideUVRect = SideUVRect;
	}

	if (bOverrideBottomUVRect)
	{
		OutParameters.BottomUVRect = BottomUVRect;
	}
}
