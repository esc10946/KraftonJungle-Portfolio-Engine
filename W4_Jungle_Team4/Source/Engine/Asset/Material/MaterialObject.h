#pragma once

#include "Object/ObjectFactory.h"
#include "Math/Vector.h"

class UTexture2D;

class UMaterial : public UObject
{
public:
	DECLARE_CLASS(UMaterial, UObject)

	UMaterial() = default;
	~UMaterial() override = default;

	void SetSlotName(const FString& InSlotName) { SlotName = InSlotName; }
	const FString& GetSlotName() const { return SlotName; }

	void SetSourceMaterialName(const FString& InSourceMaterialName) { SourceMaterialName = InSourceMaterialName; }
	const FString& GetSourceMaterialName() const { return SourceMaterialName; }

	void SetDiffuseTexturePath(const FString& InDiffuseTexturePath) { DiffuseTexturePath = InDiffuseTexturePath; }
	const FString& GetDiffuseTexturePath() const { return DiffuseTexturePath; }
	void SetDiffuseTexture(UTexture2D* InDiffuseTexture) { DiffuseTexture = InDiffuseTexture; }
	UTexture2D* GetDiffuseTexture() const { return DiffuseTexture; }

	void SetBaseColor(const FVector4& InBaseColor) { BaseColor = InBaseColor; }
	const FVector4& GetBaseColor() const { return BaseColor; }

	void SetUseAtlas(bool InUseAtlas) { bUseAtlas = InUseAtlas; }
	bool UsesAtlas() const { return bUseAtlas; }

	void SetAtlasLayout(int32 InColumns, int32 InRows)
	{
		AtlasColumns = InColumns;
		AtlasRows = InRows;
	}
	int32 GetAtlasColumns() const { return AtlasColumns; }
	int32 GetAtlasRows() const { return AtlasRows; }
	void SetTopTile(int32 InX, int32 InY) { TopTileX = InX; TopTileY = InY; }
	void SetSideTile(int32 InX, int32 InY) { SideTileX = InX; SideTileY = InY; }
	void SetBottomTile(int32 InX, int32 InY) { BottomTileX = InX; BottomTileY = InY; }
	int32 GetTopTileX() const { return TopTileX; }
	int32 GetTopTileY() const { return TopTileY; }
	int32 GetSideTileX() const { return SideTileX; }
	int32 GetSideTileY() const { return SideTileY; }
	int32 GetBottomTileX() const { return BottomTileX; }
	int32 GetBottomTileY() const { return BottomTileY; }

	void SetTopUVRect(const FVector4& InRect) { TopUVRect = InRect; }
	void SetSideUVRect(const FVector4& InRect) { SideUVRect = InRect; }
	void SetBottomUVRect(const FVector4& InRect) { BottomUVRect = InRect; }
	const FVector4& GetTopUVRect() const { return TopUVRect; }
	const FVector4& GetSideUVRect() const { return SideUVRect; }
	const FVector4& GetBottomUVRect() const { return BottomUVRect; }

private:
	FString SlotName;
	FString SourceMaterialName;
	FString DiffuseTexturePath;
	UTexture2D* DiffuseTexture = nullptr;
	FVector4 BaseColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

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
