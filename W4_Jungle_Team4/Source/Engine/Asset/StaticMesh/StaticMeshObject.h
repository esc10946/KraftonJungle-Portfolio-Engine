#pragma once

#include "Object/ObjectFactory.h"

struct FStaticMeshAsset;

class UStaticMesh : public UObject
{
public:
	DECLARE_CLASS(UStaticMesh, UObject)

	UStaticMesh() = default;
	~UStaticMesh() override = default;

	void SetStaticMeshAsset(FStaticMeshAsset* InStaticMeshAsset);

	FStaticMeshAsset* GetStaticMeshAsset() const { return StaticMeshAsset; }
	const FString& GetAssetPathFileName() const;
	FString GetDisplayName() const;
	bool HasStaticMeshAsset() const { return StaticMeshAsset != nullptr; }

private:
	FStaticMeshAsset* StaticMeshAsset = nullptr;
};
