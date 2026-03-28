#include "StaticMeshComponent.h"

#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/StaticMesh/StaticMeshManager.h"
#include "Asset/StaticMesh/StaticMesh.h"
#include "Render/Resource/Material.h"
#include "Math/Vector.h"

#include <algorithm>
#include <cstring>
#include <Core/ResourceManager.h>

DEFINE_CLASS(UStaticMeshComponent, UMeshComponent)
REGISTER_FACTORY(UStaticMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
    const FString ObjPath = "Data/cube-tex.obj";

    // 1. FStaticMeshAsset 로드
    FStaticMeshAsset* LoadedAsset = FStaticMeshManager::Get().LoadStaticMeshAsset(ObjPath);
    if (!LoadedAsset) return;

    // 2. UStaticMesh 생성
    UStaticMesh* NewMesh = UObjectManager::Get().CreateObject<UStaticMesh>();
    NewMesh->SetMeshAsset(*LoadedAsset);
    NewMesh->MarkGPUDirty();
    SetStaticMesh(NewMesh);

    // 3. cube-tex.mtl 내용 그대로 하드코딩
    // newmtl texture
    // Ka 0.0 0.0 0.0
    // Kd 0.5 0.5 0.5
    // Ks 0.0 0.0 0.0
    // Ns 10.0
    // map_Kd cube-tex.png
    FMaterial MatData;
    MatData.MaterialName       = "texture";
    MatData.Ka                 = FVector(0.0f, 0.0f, 0.0f);
    MatData.Kd                 = FVector(0.5f, 0.5f, 0.5f);
    MatData.Ks                 = FVector(0.0f, 0.0f, 0.0f);
    MatData.Ns                 = 10.0f;
    MatData.map_Kd = "Data/cube-tex.png";

    // 4. UMaterial 생성
    UMaterial* Material = UObjectManager::Get().CreateObject<UMaterial>();
    Material->SetFName(MatData.MaterialName);
    Material->SetMaterialData(MatData);

    // 5. UTexture 로드 후 연결
    UTexture* DiffuseTex = FResourceManager::Get().LoadTexture(MatData.map_Kd);
    if (DiffuseTex)
        Material->SetDiffuseTexture(DiffuseTex);

	//버퍼초기화 샤갈...

    // 6. 슬롯 0번에 등록 (cube-tex.obj는 섹션 1개)
    SetMaterial(0, Material);
}

UStaticMeshComponent::~UStaticMeshComponent()
{
	
}

//에셋 교체 시
void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InMesh)
{
	StaticMesh = InMesh;
	//RefreshMeshData();
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
	return StaticMesh->IsValid() ? static_cast<int32>(StaticMesh->GetMeshAsset().Sections.size()) : 0;
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
		//RefreshMeshData();
	}
}

