#pragma once

#include "Object/ObjectFactory.h"
#include "Math/Vector.h"

class UTexture2D;
class UShader;

struct FResolvedMaterialParameters
{
	UShader* Shader = nullptr;
	UTexture2D* DiffuseTexture = nullptr;
	UTexture2D* NormalTexture = nullptr;
	FVector4 BaseColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	float Metallic = 0.0f;
	bool bUseAtlas = false;
	int32 AtlasColumns = 1;
	int32 AtlasRows = 1;
	int32 TopTileX = 0;
	int32 TopTileY = 0;
	int32 SideTileX = 0;
	int32 SideTileY = 0;
	int32 BottomTileX = 0;
	int32 BottomTileY = 0;
	FVector4 TopUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
	FVector4 SideUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
	FVector4 BottomUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
};

class UMaterialInterface : public UObject
{
public:
	DECLARE_CLASS(UMaterialInterface, UObject)

	UMaterialInterface() = default;
	~UMaterialInterface() override = default;

	void SetSlotName(const FString& InSlotName) { SlotName = InSlotName; }
	virtual const FString& GetSlotName() const { return SlotName; }

	void SetSourceMaterialName(const FString& InSourceMaterialName) { SourceMaterialName = InSourceMaterialName; }
	virtual const FString& GetSourceMaterialName() const { return SourceMaterialName; }

	void SetHiddenInMaterialPicker(bool bInHidden) { bHiddenInMaterialPicker = bInHidden; }
	bool IsHiddenInMaterialPicker() const { return bHiddenInMaterialPicker; }

	virtual const FString& GetDiffuseTexturePath() const = 0;
	virtual UTexture2D* GetDiffuseTexture() const = 0;
	virtual const FString& GetNormalTexturePath() const = 0;
	virtual UTexture2D* GetNormalTexture() const = 0;
	virtual UShader* GetShader() const = 0;

	virtual const FVector4& GetBaseColor() const = 0;
	virtual float GetMetallic() const = 0;

	virtual bool UsesAtlas() const = 0;
	virtual int32 GetAtlasColumns() const = 0;
	virtual int32 GetAtlasRows() const = 0;
	virtual int32 GetTopTileX() const = 0;
	virtual int32 GetTopTileY() const = 0;
	virtual int32 GetSideTileX() const = 0;
	virtual int32 GetSideTileY() const = 0;
	virtual int32 GetBottomTileX() const = 0;
	virtual int32 GetBottomTileY() const = 0;
	virtual const FVector4& GetTopUVRect() const = 0;
	virtual const FVector4& GetSideUVRect() const = 0;
	virtual const FVector4& GetBottomUVRect() const = 0;

	virtual void ResolveParameters(FResolvedMaterialParameters& OutParameters) const = 0;

protected:
	FString SlotName;
	FString SourceMaterialName;
	bool bHiddenInMaterialPicker = false;
};

class UMaterial : public UMaterialInterface
{
public:
	DECLARE_CLASS(UMaterial, UMaterialInterface)

	UMaterial() = default;
	~UMaterial() override = default;

	void SetDiffuseTexturePath(const FString& InDiffuseTexturePath) { DiffuseTexturePath = InDiffuseTexturePath; }
	const FString& GetDiffuseTexturePath() const override { return DiffuseTexturePath; }
	void SetDiffuseTexture(UTexture2D* InDiffuseTexture) { DiffuseTexture = InDiffuseTexture; }
	UTexture2D* GetDiffuseTexture() const override { return DiffuseTexture; }

	void SetNormalTexturePath(const FString& InNormalTexturePath) { NormalTexturePath = InNormalTexturePath; }
	const FString& GetNormalTexturePath() const override { return NormalTexturePath; }
	void SetNormalTexture(UTexture2D* InNormalTexture) { NormalTexture = InNormalTexture; }
	UTexture2D* GetNormalTexture() const override { return NormalTexture; }

	void SetShader(UShader* InShader) { Shader = InShader; }
	UShader* GetShader() const override { return Shader; }

	void SetBaseColor(const FVector4& InBaseColor) { BaseColor = InBaseColor; }
	const FVector4& GetBaseColor() const override { return BaseColor; }
	void SetMetallic(float InMetallic) { Metallic = InMetallic; }
	float GetMetallic() const override { return Metallic; }

	void SetUseAtlas(bool InUseAtlas) { bUseAtlas = InUseAtlas; }
	bool UsesAtlas() const override { return bUseAtlas; }

	void SetAtlasLayout(int32 InColumns, int32 InRows)
	{
		AtlasColumns = InColumns;
		AtlasRows = InRows;
	}
	int32 GetAtlasColumns() const override { return AtlasColumns; }
	int32 GetAtlasRows() const override { return AtlasRows; }
	void SetTopTile(int32 InX, int32 InY) { TopTileX = InX; TopTileY = InY; }
	void SetSideTile(int32 InX, int32 InY) { SideTileX = InX; SideTileY = InY; }
	void SetBottomTile(int32 InX, int32 InY) { BottomTileX = InX; BottomTileY = InY; }
	int32 GetTopTileX() const override { return TopTileX; }
	int32 GetTopTileY() const override { return TopTileY; }
	int32 GetSideTileX() const override { return SideTileX; }
	int32 GetSideTileY() const override { return SideTileY; }
	int32 GetBottomTileX() const override { return BottomTileX; }
	int32 GetBottomTileY() const override { return BottomTileY; }

	void SetTopUVRect(const FVector4& InRect) { TopUVRect = InRect; }
	void SetSideUVRect(const FVector4& InRect) { SideUVRect = InRect; }
	void SetBottomUVRect(const FVector4& InRect) { BottomUVRect = InRect; }
	const FVector4& GetTopUVRect() const override { return TopUVRect; }
	const FVector4& GetSideUVRect() const override { return SideUVRect; }
	const FVector4& GetBottomUVRect() const override { return BottomUVRect; }

	void ResolveParameters(FResolvedMaterialParameters& OutParameters) const override;

private:
	FString DiffuseTexturePath;
	UTexture2D* DiffuseTexture = nullptr;
	FString NormalTexturePath;
	UTexture2D* NormalTexture = nullptr;
	UShader* Shader = nullptr;
	FVector4 BaseColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	float Metallic = 0.0f;

	bool bUseAtlas = false;
	int32 AtlasColumns = 1;
	int32 AtlasRows = 1;

	int32 TopTileX = 0;
	int32 TopTileY = 0;
	int32 SideTileX = 0;
	int32 SideTileY = 0;
	int32 BottomTileX = 0;
	int32 BottomTileY = 0;

	FVector4 TopUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
	FVector4 SideUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
	FVector4 BottomUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
};

class UMaterialInstance : public UMaterialInterface
{
public:
	DECLARE_CLASS(UMaterialInstance, UMaterialInterface)

	UMaterialInstance() = default;
	~UMaterialInstance() override = default;

	void SetParentMaterial(UMaterial* InParentMaterial) { ParentMaterial = InParentMaterial; }
	UMaterial* GetParentMaterial() const { return ParentMaterial; }

	void SetTextureParameter(UTexture2D* InTexture);
	void ClearTextureParameter();
	void SetNormalTextureParameter(UTexture2D* InTexture);
	void ClearNormalTextureParameter();
	void SetShaderParameter(UShader* InShader);
	void ClearShaderParameter();
	void SetBaseColorParameter(const FVector4& InColor);
	void ClearBaseColorParameter();
	void SetMetallicParameter(float InMetallic);
	void ClearMetallicParameter();
	void SetTopUVRectParameter(const FVector4& InRect);
	void SetSideUVRectParameter(const FVector4& InRect);
	void SetBottomUVRectParameter(const FVector4& InRect);
	void ClearUVRectParameters();

	const FString& GetSlotName() const override;
	const FString& GetSourceMaterialName() const override;
	const FString& GetDiffuseTexturePath() const override;
	UTexture2D* GetDiffuseTexture() const override;
	const FString& GetNormalTexturePath() const override;
	UTexture2D* GetNormalTexture() const override;
	UShader* GetShader() const override;
	const FVector4& GetBaseColor() const override;
	float GetMetallic() const override;
	bool UsesAtlas() const override;
	int32 GetAtlasColumns() const override;
	int32 GetAtlasRows() const override;
	int32 GetTopTileX() const override;
	int32 GetTopTileY() const override;
	int32 GetSideTileX() const override;
	int32 GetSideTileY() const override;
	int32 GetBottomTileX() const override;
	int32 GetBottomTileY() const override;
	const FVector4& GetTopUVRect() const override;
	const FVector4& GetSideUVRect() const override;
	const FVector4& GetBottomUVRect() const override;
	void ResolveParameters(FResolvedMaterialParameters& OutParameters) const override;

	FVector4* GetEditableBaseColorPtr(){
		if (!bOverrideBaseColor)
		{
			BaseColor = GetBaseColor();
			bOverrideBaseColor = true;
		}
		return &BaseColor;
	}

private:
	UMaterial* ParentMaterial = nullptr;
	FString DiffuseTexturePath;
	UTexture2D* DiffuseTexture = nullptr;
	FString NormalTexturePath;
	UTexture2D* NormalTexture = nullptr;
	UShader* Shader = nullptr;
	FVector4 BaseColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	float Metallic = 0.0f;
	FVector4 TopUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
	FVector4 SideUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
	FVector4 BottomUVRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f);

	bool bOverrideDiffuseTexture = false;
	bool bOverrideNormalTexture = false;
	bool bOverrideShader = false;
	bool bOverrideBaseColor = false;
	bool bOverrideMetallic = false;
	bool bOverrideTopUVRect = false;
	bool bOverrideSideUVRect = false;
	bool bOverrideBottomUVRect = false;
};
