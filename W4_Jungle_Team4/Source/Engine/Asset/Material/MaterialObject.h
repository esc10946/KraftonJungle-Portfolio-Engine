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

private:
	FString SlotName;
	FString SourceMaterialName;
	FString DiffuseTexturePath;
	UTexture2D* DiffuseTexture = nullptr;
	FVector4 BaseColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
};
