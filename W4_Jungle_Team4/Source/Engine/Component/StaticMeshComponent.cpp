#include "StaticMeshComponent.h"

#include "Asset/Material/MaterialObject.h"
#include "Asset/StaticMesh/StaticMeshAsset.h"
#include "Asset/StaticMesh/StaticMeshObject.h"
#include "Math/Vector.h"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <limits>

DEFINE_CLASS(UStaticMeshComponent, UMeshComponent)
REGISTER_FACTORY(UStaticMeshComponent)

namespace
{
	struct FFaceGroup
	{
		FVector Normal;
		TArray<int32> VertexIndices;
		float MinU = std::numeric_limits<float>::max();
		float MaxU = std::numeric_limits<float>::lowest();
		float MinV = std::numeric_limits<float>::max();
		float MaxV = std::numeric_limits<float>::lowest();
	};

	bool SameNormal(const FVector& A, const FVector& B)
	{
		return std::fabs(A.X - B.X) < 0.001f &&
			std::fabs(A.Y - B.Y) < 0.001f &&
			std::fabs(A.Z - B.Z) < 0.001f;
	}
}

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

const FTexturedMeshData* UStaticMeshComponent::GetTexturedMeshData() const
{
	return StaticMeshTexturedRenderData.Vertices.empty() ? nullptr : &StaticMeshTexturedRenderData;
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
	RefreshMeshData();
}

UMaterial* UStaticMeshComponent::GetMaterial(int32 SlotIndex) const
{
	if (SlotIndex < 0)
	{
		return nullptr;
	}

	if (SlotIndex < static_cast<int32>(OverrideMaterials.size()) && OverrideMaterials[SlotIndex])
	{
		return OverrideMaterials[SlotIndex];
	}

	if (!StaticMesh)
	{
		return nullptr;
	}

	return StaticMesh->GetDefaultMaterial(SlotIndex);
}

int32 UStaticMeshComponent::GetNumMaterials() const
{
	return StaticMesh ? StaticMesh->GetNumMaterialSlots() : 0;
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

	if (StaticMesh)
	{
		if (OverrideMaterials.size() < 1)
		{
			OverrideMaterials.resize(1, nullptr);
		}

		FPropertyDescriptor MaterialProperty;
		MaterialProperty.Name = "Material";
		MaterialProperty.Type = EPropertyType::Object;
		MaterialProperty.ValuePtr = &OverrideMaterials[0];
		MaterialProperty.ObjectClass = &UMaterial::s_TypeInfo;
		OutProps.push_back(MaterialProperty);
	}
}

void UStaticMeshComponent::PostEditProperty(const char* PropertyName)
{
	if (std::strcmp(PropertyName, "StaticMesh") == 0 ||
		std::strcmp(PropertyName, "Material") == 0)
	{
		RefreshMeshData();
	}
}

void UStaticMeshComponent::RefreshMeshData()
{
	FStaticMeshAsset* Asset = GetStaticMeshAsset();
	if (Asset &&
		Asset->BuildRenderMeshData(StaticMeshRenderData) &&
		Asset->BuildTexturedRenderMeshData(StaticMeshTexturedRenderData))
	{
		ApplyAtlasMaterialToTexturedMeshData();
		++TexturedMeshRevision;
		LocalExtents = (Asset->BoundsMax - Asset->BoundsMin) * 0.5f;
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

void UStaticMeshComponent::ApplyAtlasMaterialToTexturedMeshData()
{
	FStaticMeshAsset* Asset = GetStaticMeshAsset();
	UMaterial* Material = GetMaterial(0);

	if (!Asset || !Material || !Material->UsesAtlas())
	{
		return;
	}

	TArray<FFaceGroup> Groups;
	Groups.reserve(6);

	for (int32 VertexIndex = 0; VertexIndex < static_cast<int32>(Asset->Vertices.size()); ++VertexIndex)
	{
		const FStaticMeshVertex& SourceVertex = Asset->Vertices[VertexIndex];

		int32 GroupIndex = -1;
		for (int32 Index = 0; Index < static_cast<int32>(Groups.size()); ++Index)
		{
			if (SameNormal(Groups[Index].Normal, SourceVertex.Normal))
			{
				GroupIndex = Index;
				break;
			}
		}

		if (GroupIndex == -1)
		{
			FFaceGroup NewGroup;
			NewGroup.Normal = SourceVertex.Normal;
			Groups.push_back(NewGroup);
			GroupIndex = static_cast<int32>(Groups.size()) - 1;
		}

		FFaceGroup& Group = Groups[GroupIndex];
		Group.VertexIndices.push_back(VertexIndex);
		Group.MinU = std::min(Group.MinU, SourceVertex.TexCoord.X);
		Group.MaxU = std::max(Group.MaxU, SourceVertex.TexCoord.X);
		Group.MinV = std::min(Group.MinV, SourceVertex.TexCoord.Y);
		Group.MaxV = std::max(Group.MaxV, SourceVertex.TexCoord.Y);
	}

	for (const FFaceGroup& Group : Groups)
	{
		FVector4 TargetRect = Material->GetSideUVRect();
		/*if (Group.Normal.Y > 0.5f)
		{
			TargetRect = Material->GetTopUVRect();
		}
		else if (Group.Normal.Y < -0.5f)
		{
			TargetRect = Material->GetBottomUVRect();
		}*/
		if (Group.Normal.Z > 0.5f)
		{
			TargetRect = Material->GetTopUVRect();
		}
		else if (Group.Normal.Z < -0.5f)
		{
			TargetRect = Material->GetBottomUVRect();
		}

		const float RangeU = std::max(Group.MaxU - Group.MinU, 0.0001f);
		const float RangeV = std::max(Group.MaxV - Group.MinV, 0.0001f);

		for (int32 VertexIndex : Group.VertexIndices)
		{
			const FVector2& SourceUV = Asset->Vertices[VertexIndex].TexCoord;
			const float LocalU = (SourceUV.X - Group.MinU) / RangeU;
			const float LocalV = (SourceUV.Y - Group.MinV) / RangeV;

			StaticMeshTexturedRenderData.Vertices[VertexIndex].TexCoord = FVector2(
				TargetRect.X + LocalU * (TargetRect.Z - TargetRect.X),
				TargetRect.Y + LocalV * (TargetRect.W - TargetRect.Y));
		}
	}
}
