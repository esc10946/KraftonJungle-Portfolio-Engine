#include "StaticMeshComponent.h"

#include "Asset/Material/MaterialObject.h"
#include "Asset/Material/MaterialManager.h"
#include "Asset/StaticMesh/StaticMeshObject.h"
#include "GameFramework/PrimitiveActors.h"

#include <cstring>

namespace
{
	int32 ParseElementIndex(const char* PropertyName)
	{
		if (!PropertyName || std::strncmp(PropertyName, "Element ", 8) != 0)
		{
			return -1;
		}

		int32 ElementIndex = 0;
		bool bHasDigit = false;
		const char* Cursor = PropertyName + 8;
		while (*Cursor >= '0' && *Cursor <= '9')
		{
			bHasDigit = true;
			ElementIndex = (ElementIndex * 10) + (*Cursor - '0');
			++Cursor;
		}

		return bHasDigit ? ElementIndex : -1;
	}

	FVector4 ResolveBaseColor(UMaterialInterface* Material)
	{
		return Material ? Material->GetBaseColor() : FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	UMaterial* ResolveParentMaterial(UMaterialInterface* Material)
	{
		if (UMaterial* BaseMaterial = UMaterial::Cast(Material))
		{
			return BaseMaterial;
		}

		if (UMaterialInstance* MaterialInstance = UMaterialInstance::Cast(Material))
		{
			return MaterialInstance->GetParentMaterial();
		}

		return nullptr;
	}

	void CopyInstanceParameters(UMaterialInstance* TargetInstance, UMaterialInterface* SourceMaterial)
	{
		if (!TargetInstance || !UMaterialInstance::Cast(SourceMaterial))
		{
			return;
		}

		FResolvedMaterialParameters Parameters;
		SourceMaterial->ResolveParameters(Parameters);

		if (Parameters.DiffuseTexture)
		{
			TargetInstance->SetTextureParameter(Parameters.DiffuseTexture);
		}
		if (Parameters.NormalTexture)
		{
			TargetInstance->SetNormalTextureParameter(Parameters.NormalTexture);
		}
		if (Parameters.Shader)
		{
			TargetInstance->SetShaderParameter(Parameters.Shader);
		}

		TargetInstance->SetBaseColorParameter(Parameters.BaseColor);
		TargetInstance->SetMetallicParameter(Parameters.Metallic);
		TargetInstance->SetTopUVRectParameter(Parameters.TopUVRect);
		TargetInstance->SetSideUVRectParameter(Parameters.SideUVRect);
		TargetInstance->SetBottomUVRectParameter(Parameters.BottomUVRect);
	}

	void ResetInstanceOverrides(UMaterialInstance* MaterialInstance)
	{
		if (!MaterialInstance)
		{
			return;
		}

		MaterialInstance->ClearTextureParameter();
		MaterialInstance->ClearNormalTextureParameter();
		MaterialInstance->ClearShaderParameter();
		MaterialInstance->ClearBaseColorParameter();
		MaterialInstance->ClearMetallicParameter();
		MaterialInstance->ClearUVRectParameters();
	}
}

DEFINE_CLASS(UStaticMeshComponent, UMeshComponent)
REGISTER_FACTORY(UStaticMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
}

UStaticMeshComponent::~UStaticMeshComponent()
{
    ReleaseOwnedMaterialInstances();
}

void UStaticMeshComponent::ReleaseOwnedMaterialInstances()
{
	int32 StartIndex = 0;
    
	if (StartIndex < 0)
    {
        StartIndex = 0;
    }

    for (int32 i = StartIndex; i < static_cast<int32>(SectionMaterialInstances.size()); ++i)
    {
        if (SectionMaterialInstances[i])
        {
            UObjectManager::Get().DestroyObject(SectionMaterialInstances[i]);
            SectionMaterialInstances[i] = nullptr;
        }
    }
}


void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InMesh)
{
	StaticMesh = InMesh;
	AssignedMaterials.clear();
	SectionMaterialInstances.clear();
	EditableMaterialColors.clear();
	InitializeMaterialSlotsFromMesh();
	RefreshMeshData();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		AStaticMeshActor* OwnerActor = AStaticMeshActor::Cast(Owner);
		if (OwnerActor != nullptr)
		{
			OwnerActor->UpdateUUIDTextLocation();
		}
	}
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

	const int32 NumSections = StaticMesh ? StaticMesh->GetNumSections() : (SlotIndex + 1);
	ResizeMaterialState(NumSections);

	if (SlotIndex >= static_cast<int32>(AssignedMaterials.size()))
	{
		return;
	}

	AssignedMaterials[SlotIndex] = InMaterial;

	UMaterialInterface* EffectiveMaterial = InMaterial;
	if (!EffectiveMaterial && StaticMesh)
	{
		EffectiveMaterial = StaticMesh->GetDefaultMaterialForSection(SlotIndex);
	}
	EditableMaterialColors[SlotIndex] = ResolveBaseColor(EffectiveMaterial);

	if (UMaterialInstance* MaterialInstance = SectionMaterialInstances[SlotIndex])
	{
		UMaterial* ParentMaterial = ResolveParentMaterial(EffectiveMaterial);
		if (!ParentMaterial)
		{
			SectionMaterialInstances[SlotIndex] = nullptr;
		}
		else
		{
			MaterialInstance->SetParentMaterial(ParentMaterial);
			ResetInstanceOverrides(MaterialInstance);
			CopyInstanceParameters(MaterialInstance, EffectiveMaterial);
			MaterialInstance->SetBaseColorParameter(EditableMaterialColors[SlotIndex]);
		}
	}

	RefreshMeshData();
}

UMaterialInterface* UStaticMeshComponent::GetMaterial(int32 SlotIndex) const
{
	if (SlotIndex < 0)
	{
		return nullptr;
	}

	if (SlotIndex < static_cast<int32>(SectionMaterialInstances.size()) && SectionMaterialInstances[SlotIndex])
	{
		return SectionMaterialInstances[SlotIndex];
	}

	if (SlotIndex < static_cast<int32>(AssignedMaterials.size()) && AssignedMaterials[SlotIndex])
	{
		return AssignedMaterials[SlotIndex];
	}

	return StaticMesh ? StaticMesh->GetDefaultMaterialForSection(SlotIndex) : nullptr;
}

int32 UStaticMeshComponent::GetNumMaterials() const
{
	return StaticMesh ? StaticMesh->GetNumSections() : 0;
}

void UStaticMeshComponent::UpdateWorldAABB() const
{
	if (!StaticMesh)
	{
		UPrimitiveComponent::UpdateWorldAABB();
		return;
	}

	const FVector LocalMin = StaticMesh->GetBoundsMin();
	const FVector LocalMax = StaticMesh->GetBoundsMax();
	const FVector LocalCenter = (LocalMin + LocalMax) * 0.5f;
	const FVector LocalExtent = (LocalMax - LocalMin) * 0.5f;

	const FMatrix WorldMatrix = GetWorldMatrix();
	const FVector WorldCenter = WorldMatrix.TransformPositionWithW(LocalCenter);

	const float NewEx = std::abs(WorldMatrix.M[0][0]) * LocalExtent.X +
		std::abs(WorldMatrix.M[1][0]) * LocalExtent.Y +
		std::abs(WorldMatrix.M[2][0]) * LocalExtent.Z;
	const float NewEy = std::abs(WorldMatrix.M[0][1]) * LocalExtent.X +
		std::abs(WorldMatrix.M[1][1]) * LocalExtent.Y +
		std::abs(WorldMatrix.M[2][1]) * LocalExtent.Z;
	const float NewEz = std::abs(WorldMatrix.M[0][2]) * LocalExtent.X +
		std::abs(WorldMatrix.M[1][2]) * LocalExtent.Y +
		std::abs(WorldMatrix.M[2][2]) * LocalExtent.Z;

	WorldAABBMinLocation = WorldCenter - FVector(NewEx, NewEy, NewEz);
	WorldAABBMaxLocation = WorldCenter + FVector(NewEx, NewEy, NewEz);
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
	ResizeMaterialState(static_cast<int32>(Sections.size()));
	EditableMaterialPropertyNames.clear();
	EditableMaterialPropertyNames.reserve(Sections.size() * 2);

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
		MaterialProperty.ValuePtr = &AssignedMaterials[SectionIndex];
		MaterialProperty.ObjectClass = &UMaterialInterface::s_TypeInfo;
		OutProps.push_back(MaterialProperty);

		EditableMaterialPropertyNames.push_back(
			"Element " + std::to_string(SectionIndex) + " Color [" + SlotLabel + "]");

		FPropertyDescriptor ColorProperty;
		ColorProperty.Name = EditableMaterialPropertyNames.back().c_str();
		ColorProperty.Type = EPropertyType::Vec4;
		ColorProperty.ValuePtr = &EditableMaterialColors[SectionIndex].X;
		OutProps.push_back(ColorProperty);
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
		AssignedMaterials.clear();
		SectionMaterialInstances.clear();
		EditableMaterialColors.clear();
		InitializeMaterialSlotsFromMesh();
		RefreshMeshData();

		AActor* Owner = GetOwner();
		if (Owner)
		{
			AStaticMeshActor* OwnerActor = AStaticMeshActor::Cast(Owner);
			if (OwnerActor != nullptr)
			{
				OwnerActor->UpdateUUIDTextLocation();
			}
		}
		return;
	}

	if (std::strstr(PropertyName, "Material [") != nullptr)
	{
		const int32 SectionIndex = ParseElementIndex(PropertyName);
		if (StaticMesh &&
			SectionIndex >= 0 &&
			SectionIndex < static_cast<int32>(AssignedMaterials.size()) &&
			SectionIndex < static_cast<int32>(SectionMaterialInstances.size()) &&
			SectionIndex < static_cast<int32>(EditableMaterialColors.size()))
		{
			UMaterialInterface* EffectiveMaterial = AssignedMaterials[SectionIndex]
				? AssignedMaterials[SectionIndex]
				: StaticMesh->GetDefaultMaterialForSection(SectionIndex);
			EditableMaterialColors[SectionIndex] = ResolveBaseColor(EffectiveMaterial);

			if (UMaterialInstance* MaterialInstance = SectionMaterialInstances[SectionIndex])
			{
				UMaterial* ParentMaterial = ResolveParentMaterial(EffectiveMaterial);
				if (!ParentMaterial)
				{
					SectionMaterialInstances[SectionIndex] = nullptr;
				}
				else
				{
					MaterialInstance->SetParentMaterial(ParentMaterial);
					ResetInstanceOverrides(MaterialInstance);
					CopyInstanceParameters(MaterialInstance, EffectiveMaterial);
					MaterialInstance->SetBaseColorParameter(EditableMaterialColors[SectionIndex]);
				}
			}
		}
		RefreshMeshData();
		return;
	}

	if (std::strstr(PropertyName, "Color [") != nullptr)
	{
		const int32 SectionIndex = ParseElementIndex(PropertyName);
		if (SectionIndex >= 0 &&
			SectionIndex < static_cast<int32>(EditableMaterialColors.size()))
		{
			if (UMaterialInstance* MaterialInstance = EnsureMaterialInstance(SectionIndex))
			{
				MaterialInstance->SetBaseColorParameter(EditableMaterialColors[SectionIndex]);
			}
		}
		RefreshMeshData();
		return;
	}
}

void UStaticMeshComponent::InitializeMaterialSlotsFromMesh()
{
	EditableMaterialPropertyNames.clear();

	if (!StaticMesh)
	{
		AssignedMaterials.clear();
		SectionMaterialInstances.clear();
		EditableMaterialColors.clear();
		return;
	}

	const int32 NumSections = StaticMesh->GetNumSections();
	ResizeMaterialState(NumSections);

	for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
	{
		EditableMaterialColors[SectionIndex] =
			ResolveBaseColor(StaticMesh->GetDefaultMaterialForSection(SectionIndex));
	}
}

void UStaticMeshComponent::ResizeMaterialState(int32 NumSections)
{
	if (NumSections < 0)
	{
		NumSections = 0;
	}

	AssignedMaterials.resize(static_cast<size_t>(NumSections), nullptr);
	SectionMaterialInstances.resize(static_cast<size_t>(NumSections), nullptr);
	EditableMaterialColors.resize(static_cast<size_t>(NumSections), FVector4(1.0f, 1.0f, 1.0f, 1.0f));
}

void UStaticMeshComponent::RefreshMeshData()
{
	if (StaticMesh &&
		StaticMesh->BuildRenderMeshData(StaticMeshRenderData) &&
		StaticMesh->BuildTexturedRenderMeshData(StaticMeshTexturedRenderData))
	{
		++TexturedMeshRevision;
		MeshData = &StaticMeshRenderData;
		LocalExtents = (StaticMesh->GetBoundsMax() - StaticMesh->GetBoundsMin()) * 0.5f;
		UpdateWorldAABB();
		return;
	}

	StaticMeshRenderData.Vertices.clear();
	StaticMeshRenderData.Indices.clear();
	StaticMeshTexturedRenderData.Vertices.clear();
	StaticMeshTexturedRenderData.Indices.clear();
	MeshData = nullptr;
}

UMaterialInstance* UStaticMeshComponent::EnsureMaterialInstance(int32 SectionIndex)
{
	if (!StaticMesh || SectionIndex < 0 || SectionIndex >= StaticMesh->GetNumSections())
	{
		return nullptr;
	}

	ResizeMaterialState(StaticMesh->GetNumSections());

	if (SectionMaterialInstances[SectionIndex])
	{
		return SectionMaterialInstances[SectionIndex];
	}

	UMaterialInterface* SourceMaterial = AssignedMaterials[SectionIndex]
		? AssignedMaterials[SectionIndex]
		: StaticMesh->GetDefaultMaterialForSection(SectionIndex);
	UMaterial* ParentMaterial = ResolveParentMaterial(SourceMaterial);
	if (!ParentMaterial)
	{
		return nullptr;
	}

	UMaterialInstance* MaterialInstance = FMaterialManager::Get().CreateMaterialInstance(ParentMaterial, FString());
	MaterialInstance->SetHiddenInMaterialPicker(true);
	CopyInstanceParameters(MaterialInstance, SourceMaterial);
	MaterialInstance->SetBaseColorParameter(EditableMaterialColors[SectionIndex]);
	SectionMaterialInstances[SectionIndex] = MaterialInstance;
	return MaterialInstance;
}

