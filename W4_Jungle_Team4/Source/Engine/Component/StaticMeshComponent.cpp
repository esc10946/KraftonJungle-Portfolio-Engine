#include "StaticMeshComponent.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/StaticMesh/StaticMeshObject.h"
#include "Asset/StaticMesh/StaticMeshManager.h"
#include "Math/Vector.h"

#include <algorithm>
#include <cstring>

DEFINE_CLASS(UStaticMeshComponent, UMeshComponent)
REGISTER_FACTORY(UStaticMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
	//todo: Fname얘기하기
	//Default로 Cube설정
	FString DefaultObjPath = "Data/Shapes/Cube.obj";
	UStaticMesh* NewMesh = FStaticMeshManager::Get().LoadStaticMeshObject(DefaultObjPath);
	SetStaticMesh(NewMesh);
}

//에셋 교체 시
void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InMesh)
{
	StaticMesh = InMesh;
	RefreshMeshData();
}

FStaticMeshAsset* UStaticMeshComponent::GetStaticMeshAsset() const
{
	return StaticMesh ? StaticMesh->GetStaticMeshAsset() : nullptr;
}

FString UStaticMeshComponent::GetStaticMeshAssetPath() const
{
	return StaticMesh ? StaticMesh->GetAssetPathFileName() : FString();
}

void UStaticMeshComponent::SetMaterial(int32 SlotIndex, UMaterial* InMaterial)
{
	if (SlotIndex < 0)
	{
		return;
	}

	if (SlotIndex >= static_cast<int32>(OverrideMaterials.size()))
	{
		OverrideMaterials.resize(SlotIndex + 1, nullptr);
	}

	OverrideMaterials[SlotIndex] = InMaterial;
}

UMaterial* UStaticMeshComponent::GetMaterial(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= static_cast<int32>(OverrideMaterials.size()))
	{
		return nullptr;
	}

	return OverrideMaterials[SlotIndex];
}

int32 UStaticMeshComponent::GetNumMaterials() const
{
	const FStaticMeshAsset* Asset = GetStaticMeshAsset();
	return Asset ? static_cast<int32>(Asset->Sections.size()) : 0;
}

void UStaticMeshComponent::UpdateWorldAABB() const
{
	UPrimitiveComponent::UpdateWorldAABB();
}

void UStaticMeshComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
	UPrimitiveComponent::GetEditableProperties(OutProps);

	FPropertyDescriptor MeshProperty;
	MeshProperty.Name = "StaticMesh";
	MeshProperty.Type = EPropertyType::Object;
	MeshProperty.ValuePtr = &StaticMesh;
	MeshProperty.ObjectClass = &UStaticMesh::s_TypeInfo;
	OutProps.push_back(MeshProperty);
}

//에디터에서 Property 변경 시
void UStaticMeshComponent::PostEditProperty(const char* PropertyName)
{
	if (std::strcmp(PropertyName, "StaticMesh") == 0)
	{
		RefreshMeshData();
	}
}

void UStaticMeshComponent::RefreshMeshData()
{
	FStaticMeshAsset* Asset = GetStaticMeshAsset();
	if (Asset && Asset->BuildRenderMeshData(StaticMeshRenderData))
	{
		LocalExtents = (Asset->BoundsMax - Asset->BoundsMin) * 0.5f;
		MeshData = &StaticMeshRenderData;
		UpdateWorldAABB();
		return;
	}

	StaticMeshRenderData.Vertices.clear();
	StaticMeshRenderData.Indices.clear();
	MeshData = nullptr;
}
