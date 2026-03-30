#include "StaticMeshComponent.h"

#include "Asset/Material/MaterialObject.h"
#include "Asset/StaticMesh/StaticMeshObject.h"

#include <cstring>

DEFINE_CLASS(UStaticMeshComponent, UMeshComponent)
REGISTER_FACTORY(UStaticMeshComponent)

void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InMesh)
{
	StaticMesh = InMesh;
	OverrideMaterials.clear();
	InitializeMaterialSlotsFromMesh();
	RefreshMeshData();
}

FString UStaticMeshComponent::GetStaticMeshAssetPath() const
{
	return StaticMesh ? StaticMesh->GetAssetPathFileName() : FString();
}

const FTexturedMeshData* UStaticMeshComponent::GetTexturedMeshData() const
{
	return StaticMeshTexturedRenderData.Vertices.empty() ? nullptr : &StaticMeshTexturedRenderData;
}

void UStaticMeshComponent::SetMaterial(int32 SlotIndex, UMaterialInterface* InMaterial)
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
	RefreshMeshData();
}

UMaterialInterface* UStaticMeshComponent::GetMaterial(int32 SlotIndex) const
{
	if (SlotIndex < 0)
	{
		return nullptr;
	}

	if (SlotIndex < static_cast<int32>(OverrideMaterials.size()) && OverrideMaterials[SlotIndex])
	{
		return OverrideMaterials[SlotIndex];
	}

	return StaticMesh ? StaticMesh->GetDefaultMaterialForSection(SlotIndex) : nullptr;
}

int32 UStaticMeshComponent::GetNumMaterials() const
{
	return StaticMesh ? StaticMesh->GetNumSections() : 0;
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

	if (!StaticMesh)
	{
		return;
	}

	//InitializeMaterialSlotsFromMesh();

	const TArray<FStaticMeshSection>& Sections = StaticMesh->GetSections();
	const TArray<FStaticMaterialSlot>& MaterialSlots = StaticMesh->GetMaterialSlots();
	EditableMaterialPropertyNames.clear();
	EditableMaterialPropertyNames.reserve(Sections.size());

	for (size_t SectionIndex = 0; SectionIndex < Sections.size(); ++SectionIndex)
	{
		const FStaticMeshSection& Section = Sections[SectionIndex];
		const FStaticMaterialSlot& Slot = MaterialSlots[Section.MaterialSlotIndex];
		const FString SlotLabel = !Slot.SlotName.empty() ? Slot.SlotName : FString("Slot") + std::to_string(Section.MaterialSlotIndex);

		EditableMaterialPropertyNames.push_back(
			"Element " + std::to_string(SectionIndex) + " Material [" + SlotLabel + "]");

		FPropertyDescriptor MaterialProperty;
		MaterialProperty.Name = EditableMaterialPropertyNames.back().c_str();
		MaterialProperty.Type = EPropertyType::Object;
		MaterialProperty.ValuePtr = &OverrideMaterials[SectionIndex];
		MaterialProperty.ObjectClass = &UMaterialInterface::s_TypeInfo;
		OutProps.push_back(MaterialProperty);
	}

	OutProps.push_back({ "UseScroll", EPropertyType::Bool, &bUseUVScroll});
	if (bUseUVScroll) {
		OutProps.push_back({ "ScrollSpeedX", EPropertyType::Float, &UVScrollSpeed.X, 0.0f, 1.0f, 0.01f });
		OutProps.push_back({ "ScrollSpeedY", EPropertyType::Float, &UVScrollSpeed.Y, 0.0f, 1.0f, 0.01f });
	}
}

void UStaticMeshComponent::PostEditProperty(const char* PropertyName)
{
	if (std::strcmp(PropertyName, "StaticMesh") == 0)
	{
		OverrideMaterials.clear();
		InitializeMaterialSlotsFromMesh();
		RefreshMeshData();
		return;
	}

	if (std::strstr(PropertyName, "Material [") != nullptr)
	{
		RefreshMeshData();
	}
}

void UStaticMeshComponent::InitializeMaterialSlotsFromMesh()
{
	EditableMaterialPropertyNames.clear();

	if (!StaticMesh)
	{
		OverrideMaterials.clear();
		return;
	}
	int32 NumSlots = StaticMesh->GetNumMaterialSlots();
	OverrideMaterials.resize(static_cast<size_t>(NumSlots), nullptr);
		for (int32 i = 0 ; i < NumSlots; i++) {
			OverrideMaterials[i] = StaticMesh->GetMaterialSlots()[i].DefaultMaterial;
		}
}

void UStaticMeshComponent::RefreshMeshData()
{
	if (StaticMesh &&
		StaticMesh->BuildRenderMeshData(StaticMeshRenderData) &&
		StaticMesh->BuildTexturedRenderMeshData(StaticMeshTexturedRenderData))
	{
		++TexturedMeshRevision;
		LocalExtents = (StaticMesh->GetBoundsMax() - StaticMesh->GetBoundsMin()) * 0.5f;
		MeshData = &StaticMeshRenderData;
		UpdateWorldAABB();
		return;
	}

	StaticMeshRenderData.Vertices.clear();
	StaticMeshRenderData.Indices.clear();
	StaticMeshTexturedRenderData.Vertices.clear();
	StaticMeshTexturedRenderData.Indices.clear();
	MeshData = nullptr;
}
