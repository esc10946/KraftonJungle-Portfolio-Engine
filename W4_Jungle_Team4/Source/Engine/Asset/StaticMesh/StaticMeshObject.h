#pragma once

#include "Object/ObjectFactory.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"

// UStaticMesh는 "엔진의 UObject로 감싼 정적 메시"
// 실제 vertex/index 데이터는 FStaticMeshAsset이 들고 있고,
// UStaticMesh는 UObject 시스템에서 다루기 위한 wrapper 역할
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
	const TArray<FStaticMaterialSlot>& GetMaterialSlots() const { return MaterialSlots; }
	int32 GetNumMaterialSlots() const { return static_cast<int32>(MaterialSlots.size()); }
	UMaterial* GetDefaultMaterial(int32 SlotIndex) const;
	void SetDefaultMaterial(int32 SlotIndex, UMaterial* InMaterial);

private:
	FStaticMeshAsset* StaticMeshAsset = nullptr;
	TArray<FStaticMaterialSlot> MaterialSlots;
};
