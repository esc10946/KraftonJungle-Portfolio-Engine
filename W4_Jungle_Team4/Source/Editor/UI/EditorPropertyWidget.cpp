#include "Editor/UI/EditorPropertyWidget.h"

#include "Editor/EditorEngine.h"

#include "ImGui/imgui.h"
#include "Component/GizmoComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SceneComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Asset/Material/MaterialObject.h"
#include "Asset/StaticMesh/StaticMeshManager.h"
#include "Asset/StaticMesh/StaticMeshObject.h"
#include "Asset/Texture/TextureObject.h"
#include "Core/PropertyTypes.h"
#include "Core/ResourceManager.h"
#include "Core/Paths.h"
#include "Object/FName.h"
#include "Object/Object.h"
#include "Runtime/Engine.h"

#include <filesystem>
#include <cstring>

#define SEPARATOR(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

namespace
{
	TArray<FString> ScanStaticMeshAssetOptions()
	{
		TArray<FString> AssetPaths;
		const std::filesystem::path DataDir =
			std::filesystem::path(FPaths::ToUtf8(FPaths::RootDir())) / "Data";

		std::error_code ErrorCode;
		if (!std::filesystem::exists(DataDir, ErrorCode))
		{
			return AssetPaths;
		}

		for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataDir, ErrorCode))
		{
			if (ErrorCode)
			{
				break;
			}

			if (!Entry.is_regular_file())
			{
				continue;
			}

			if (Entry.path().extension() == ".obj")
			{
				AssetPaths.push_back(Entry.path().lexically_normal().generic_string());
			}
		}

		return AssetPaths;
	}

	const TArray<FString>& GetStaticMeshAssetOptions()
	{
		static TArray<FString> CachedAssetPaths = ScanStaticMeshAssetOptions();
		return CachedAssetPaths;
	}

	FString GetStaticMeshAssetDisplayName(const FString& AssetPath)
	{
		const std::filesystem::path Path(AssetPath);
		return Path.filename().generic_string();
	}

	bool ContainsString(const TArray<FString>& Values, const FString& Target)
	{
		for (const FString& Value : Values)
		{
			if (Value == Target)
			{
				return true;
			}
		}

		return false;
	}

	bool ContainsMaterial(const TArray<UMaterialInterface*>& Values, UMaterialInterface* Target)
	{
		for (UMaterialInterface* Value : Values)
		{
			if (Value == Target)
			{
				return true;
			}
		}

		return false;
	}

	FString ResolveMaterialTexturePath(UMaterialInterface* Material)
	{
		if (!Material)
		{
			return FString();
		}

		FResolvedMaterialParameters Parameters;
		Material->ResolveParameters(Parameters);
		if (Parameters.DiffuseTexture)
		{
			return Parameters.DiffuseTexture->GetSourcePath();
		}

		return Material->GetDiffuseTexturePath();
	}

	FString GetTextureDisplayName(const FString& TexturePath)
	{
		if (TexturePath.empty())
		{
			return FString();
		}

		const std::filesystem::path Path(TexturePath);
		const FString Filename = Path.filename().generic_string();
		return Filename.empty() ? TexturePath : Filename;
	}

	UTexture2D* ResolveMaterialDiffuseTexture(UMaterialInterface* Material)
	{
		if (!Material)
		{
			return nullptr;
		}

		FResolvedMaterialParameters Parameters;
		Material->ResolveParameters(Parameters);
		return Parameters.DiffuseTexture ? Parameters.DiffuseTexture : Material->GetDiffuseTexture();
	}

	ID3D11ShaderResourceView* ResolveMaterialTextureSRV(UMaterialInterface* Material)
	{
		UTexture2D* Texture = ResolveMaterialDiffuseTexture(Material);
		if (!Texture || !GEngine)
		{
			return nullptr;
		}

		return Texture->GetOrCreateShaderResourceView(GEngine->GetRenderer().GetFD3DDevice().GetDevice());
	}

	FString GetMaterialDisplayLabel(UMaterialInterface* Material)
	{
		if (!Material)
		{
			return FString("None");
		}

		FString Label = Material->GetFName().ToString();
		if (Label.empty())
		{
			Label = Material->GetTypeInfo()->name;
		}

		const FString TextureLabel = GetTextureDisplayName(ResolveMaterialTexturePath(Material));
		if (!TextureLabel.empty())
		{
			Label += " [";
			Label += TextureLabel;
			Label += "]";
		}

		return Label;
	}

	void RenderMaterialTexturePreview(UMaterialInterface* Material, const char* IdSuffix, float Size)
	{
		ID3D11ShaderResourceView* TextureSRV = ResolveMaterialTextureSRV(Material);
		if (!TextureSRV)
		{
			return;
		}

		ImGui::PushID(IdSuffix);
		ImGui::Image((ImTextureID)(intptr_t)TextureSRV, ImVec2(Size, Size));
		ImGui::PopID();
	}

	bool IsPreviewMaterial(const UMaterialInterface* Material)
	{
		if (!Material)
		{
			return false;
		}

		const FString Name = Material->GetFName().ToString();
		const FString SlotName = Material->GetSlotName();
		const FString SourceName = Material->GetSourceMaterialName();
		return Name.rfind("Preview_", 0) == 0 ||
			SlotName.rfind("Preview_", 0) == 0 ||
			SourceName.rfind("Preview_", 0) == 0;
	}

	bool IsAtlasMaterial(const UMaterialInterface* Material)
	{
		if (!Material)
		{
			return false;
		}

		const FString Name = Material->GetFName().ToString();
		const FString SlotName = Material->GetSlotName();
		const FString SourceName = Material->GetSourceMaterialName();
		return Name.rfind("Atlas_", 0) == 0 ||
			SlotName.rfind("Atlas_", 0) == 0 ||
			SourceName.rfind("Atlas_", 0) == 0 ||
			Name == "Atlas_BlockMaster" ||
			SlotName == "Atlas_BlockMaster" ||
			SourceName == "Atlas_BlockMaster";
	}

	int32 ParseElementIndex(const char* PropertyName)
	{
		if (!PropertyName || std::strncmp(PropertyName, "Element ", 8) != 0)
		{
			return -1;
		}

		int32 ElementIndex = 0;
		const char* Cursor = PropertyName + 8;
		bool bHasDigit = false;
		while (*Cursor >= '0' && *Cursor <= '9')
		{
			bHasDigit = true;
			ElementIndex = (ElementIndex * 10) + (*Cursor - '0');
			++Cursor;
		}

		return bHasDigit ? ElementIndex : -1;
	}

	TArray<UMaterialInterface*> GetRelevantMaterialOptions(UStaticMeshComponent* StaticMeshComponent, int32 ElementIndex, UMaterialInterface* CurrentMaterial)
	{
		TArray<UMaterialInterface*> CandidateMaterials;

		if (CurrentMaterial)
		{
			CandidateMaterials.push_back(CurrentMaterial);
		}

		if (!StaticMeshComponent || ElementIndex < 0)
		{
			return CandidateMaterials;
		}

		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
		if (!StaticMesh)
		{
			return CandidateMaterials;
		}

		TArray<FString> RelevantTexturePaths;
		const TArray<FStaticMeshSection>& Sections = StaticMesh->GetSections();
		for (size_t SectionIndex = 0; SectionIndex < Sections.size(); ++SectionIndex)
		{
			if (UMaterialInterface* Material = StaticMesh->GetDefaultMaterialForSection(static_cast<int32>(SectionIndex)))
			{
				const FString TexturePath = ResolveMaterialTexturePath(Material);
				if (!TexturePath.empty() && !ContainsString(RelevantTexturePaths, TexturePath))
				{
					RelevantTexturePaths.push_back(TexturePath);
				}
			}
		}

		const FString CurrentTexturePath = ResolveMaterialTexturePath(CurrentMaterial);
		if (!CurrentTexturePath.empty() && !ContainsString(RelevantTexturePaths, CurrentTexturePath))
		{
			RelevantTexturePaths.push_back(CurrentTexturePath);
		}

		for (FObjectIterator It(&UMaterialInterface::s_TypeInfo); It; ++It)
		{
			UMaterialInterface* Candidate = static_cast<UMaterialInterface*>(*It);
			if (!Candidate || ContainsMaterial(CandidateMaterials, Candidate))
			{
				continue;
			}

			if (IsPreviewMaterial(Candidate))
			{
				continue;
			}

			const FString TexturePath = ResolveMaterialTexturePath(Candidate);
			const bool bTextureMatches = !TexturePath.empty() && ContainsString(RelevantTexturePaths, TexturePath);
			if (bTextureMatches || IsAtlasMaterial(Candidate))
			{
				CandidateMaterials.push_back(Candidate);
			}
		}

		return CandidateMaterials;
	}

	UMaterialInterface* ResolveSectionPreviewMaterial(UStaticMeshComponent* StaticMeshComponent, int32 ElementIndex, UMaterialInterface* CurrentMaterial)
	{
		if (CurrentMaterial)
		{
			return CurrentMaterial;
		}

		if (!StaticMeshComponent || ElementIndex < 0)
		{
			return nullptr;
		}

		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
		if (!StaticMesh)
		{
			return nullptr;
		}

		return StaticMesh->GetDefaultMaterialForSection(ElementIndex);
	}
}

void FEditorPropertyWidget::Render(float DeltaTime)
{
	(void)DeltaTime;

	ImGui::SetNextWindowSize(ImVec2(350.0f, 500.0f), ImGuiCond_Once);

	ImGui::Begin("Jungle Property Window");

	FSelectionManager& Selection = EditorEngine->GetSelectionManager();
	AActor* PrimaryActor = Selection.GetPrimarySelection();
	if (!PrimaryActor)
	{
		SelectedComponent = nullptr;
		LastSelectedActor = nullptr;
		bActorSelected = true;
		ImGui::Text("No object selected.");
		ImGui::End();
		return;
	}

	// Actor 선택이 바뀌면 초기화
	if (PrimaryActor != LastSelectedActor)
	{
		SelectedComponent = nullptr;
		LastSelectedActor = PrimaryActor;
		bActorSelected = true;
	}

	const TArray<AActor*>& SelectedActors = Selection.GetSelectedActors();
	const int32 SelectionCount = static_cast<int32>(SelectedActors.size());

	// ========== 고정 영역: Actor Info (clickable) ==========
	if (SelectionCount > 1)
	{
		ImGui::Text("Class: %s", PrimaryActor->GetTypeInfo()->name);

		FString PrimaryName = PrimaryActor->GetFName().ToString();
		if (PrimaryName.empty()) PrimaryName = PrimaryActor->GetTypeInfo()->name;

		bool bHighlight = bActorSelected;
		if (bHighlight) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
		ImGui::Text("Name: %s (+%d)", PrimaryName.c_str(), SelectionCount - 1);
		if (bHighlight) ImGui::PopStyleColor();
		if (ImGui::IsItemClicked())
		{
			bActorSelected = true;
			SelectedComponent = nullptr;
		}
		ImGui::SameLine();
		char RemoveLabel[64];
		snprintf(RemoveLabel, sizeof(RemoveLabel), "Remove %d Objects", SelectionCount);
		if (ImGui::Button(RemoveLabel))
		{
			for (AActor* Actor : SelectedActors)
			{
				if (Actor && Actor->GetWorld())
				{
					Actor->GetWorld()->DestroyActor(Actor);
				}
			}
			Selection.ClearSelection();
			SelectedComponent = nullptr;
			LastSelectedActor = nullptr;
			ImGui::End();
			return;
		}
	}
	else
	{
		ImGui::Text("Class: %s", PrimaryActor->GetTypeInfo()->name);

		// Actor 이름: 클릭 가능, 선택 시 하이라이트
		bool bHighlight = bActorSelected;
		if (bHighlight) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
		ImGui::Text("Name: %s", PrimaryActor->GetFName().ToString().c_str());
		if (bHighlight) ImGui::PopStyleColor();
		if (ImGui::IsItemClicked())
		{
			bActorSelected = true;
			SelectedComponent = nullptr;
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove"))
		{
			if (PrimaryActor->GetWorld())
			{
				PrimaryActor->GetWorld()->DestroyActor(PrimaryActor);
			}
			Selection.ClearSelection();
			SelectedComponent = nullptr;
			LastSelectedActor = nullptr;
			ImGui::End();
			return;
		}
	}

	// ========== 고정 영역: Component Tree ==========
	SEPARATOR();
	RenderComponentTree(PrimaryActor);

	// ========== 스크롤 영역: Details ==========
	SEPARATOR();
	ImGui::Text("Details");
	ImGui::Separator();

	float ScrollHeight = ImGui::GetContentRegionAvail().y;
	if (ScrollHeight < 50.0f) ScrollHeight = 50.0f;

	ImGui::BeginChild("##Details", ImVec2(0, ScrollHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
	{
		RenderDetails(PrimaryActor, SelectedActors);
	}
	ImGui::EndChild();

	ImGui::End();
}

void FEditorPropertyWidget::RenderDetails(AActor* PrimaryActor, const TArray<AActor*>& SelectedActors)
{
	if (bActorSelected)
	{
		RenderActorProperties(PrimaryActor, SelectedActors);
	}
	else if (SelectedComponent)
	{
		RenderComponentProperties();
	}
	else
	{
		ImGui::TextDisabled("Select an actor or component to view details.");
	}
}

void FEditorPropertyWidget::RenderActorProperties(AActor* PrimaryActor, const TArray<AActor*>& SelectedActors)
{
	ImGui::Text("Actor: %s", PrimaryActor->GetTypeInfo()->name);
	ImGui::Text("Name: %s", PrimaryActor->GetFName().ToString().c_str());

	if (PrimaryActor->GetRootComponent())
	{
		ImGui::Separator();
		ImGui::Text("Transform");
		ImGui::Spacing();

		FVector Pos = PrimaryActor->GetActorLocation();
		float PosArray[3] = { Pos.X, Pos.Y, Pos.Z };

		FVector Rot = PrimaryActor->GetActorRotation();
		float RotArray[3] = { Rot.X, Rot.Y, Rot.Z };

		FVector Scale = PrimaryActor->GetActorScale();
		float ScaleArray[3] = { Scale.X, Scale.Y, Scale.Z };

		if (ImGui::DragFloat3("Location", PosArray, 0.1f))
		{
			FVector Delta = FVector(PosArray[0], PosArray[1], PosArray[2]) - Pos;
			for (AActor* Actor : SelectedActors)
			{
				if (Actor) Actor->AddActorWorldOffset(Delta);
			}
			EditorEngine->GetGizmo()->UpdateGizmoTransform();
		}
		if (ImGui::DragFloat3("Rotation", RotArray, 0.1f))
		{
			FVector Delta = FVector(RotArray[0], RotArray[1], RotArray[2]) - Rot;
			for (AActor* Actor : SelectedActors)
			{
				if (Actor) Actor->SetActorRotation(Actor->GetActorRotation() + Delta);
			}
			EditorEngine->GetGizmo()->UpdateGizmoTransform();
		}
		if (ImGui::DragFloat3("Scale", ScaleArray, 0.1f))
		{
			FVector Delta = FVector(ScaleArray[0], ScaleArray[1], ScaleArray[2]) - Scale;
			for (AActor* Actor : SelectedActors)
			{
				if (Actor) Actor->SetActorScale(Actor->GetActorScale() + Delta);
			}
		}
	}

	ImGui::Separator();
	bool bVisible = PrimaryActor->IsVisible();
	if (ImGui::Checkbox("Visible", &bVisible))
	{
		PrimaryActor->SetVisible(bVisible);
	}
}

void FEditorPropertyWidget::RenderComponentTree(AActor* Actor)
{
	ImGui::Text("Components");
	ImGui::Separator();

	USceneComponent* Root = Actor->GetRootComponent();

	if (Root)
	{
		RenderSceneComponentNode(Root);
	}

	// Non-scene ActorComponents
	for (UActorComponent* Comp : Actor->GetComponents())
	{
		if (!Comp) continue;
		if (Comp->IsA<USceneComponent>()) continue;

		FString Name = Comp->GetFName().ToString();
		if (Name.empty()) Name = Comp->GetTypeInfo()->name;

		ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		if (!bActorSelected && SelectedComponent == Comp)
			Flags |= ImGuiTreeNodeFlags_Selected;

		ImGui::TreeNodeEx(Comp, Flags, "[%s] %s", Comp->GetTypeInfo()->name, Name.c_str());
		if (ImGui::IsItemClicked())
		{
			SelectedComponent = Comp;
			bActorSelected = false;
		}
	}
}

void FEditorPropertyWidget::RenderSceneComponentNode(USceneComponent* Comp)
{
	if (!Comp) return;

	FString Name = Comp->GetFName().ToString();
	if (Name.empty()) Name = Comp->GetTypeInfo()->name;

	const auto& Children = Comp->GetChildren();
	bool bHasChildren = !Children.empty();

	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
	if (!bHasChildren)
		Flags |= ImGuiTreeNodeFlags_Leaf;
	if (!bActorSelected && SelectedComponent == Comp)
		Flags |= ImGuiTreeNodeFlags_Selected;

	bool bIsRoot = (Comp->GetParent() == nullptr);
	bool bOpen = ImGui::TreeNodeEx(
		Comp, Flags, "%s%s (%s)",
		bIsRoot ? "[Root] " : "",
		Name.c_str(),
		Comp->GetTypeInfo()->name
	);

	if (ImGui::IsItemClicked())
	{
		SelectedComponent = Comp;
		bActorSelected = false;
	}

	if (bOpen)
	{
		for (USceneComponent* Child : Children)
		{
			RenderSceneComponentNode(Child);
		}
		ImGui::TreePop();
	}
}

void FEditorPropertyWidget::RenderComponentProperties()
{
	ImGui::Text("Component: %s", SelectedComponent->GetTypeInfo()->name);
	ImGui::Text("Name: %s", SelectedComponent->GetFName().ToString().c_str());
	ImGui::Separator();
	bAbortPropertyRender = false;

	// PropertyDescriptor 기반 자동 위젯 렌더링
	TArray<FPropertyDescriptor> Props;
	SelectedComponent->GetEditableProperties(Props);

	bool bIsRoot = false;
	if (SelectedComponent->IsA<USceneComponent>())
	{
		USceneComponent* SceneComp = static_cast<USceneComponent*>(SelectedComponent);
		bIsRoot = (SceneComp->GetParent() == nullptr);
	}

	// Transform 프로퍼티 이름 목록
	auto IsTransformProp = [](const char* Name) {
		return strcmp(Name, "Location") == 0
			|| strcmp(Name, "Rotation") == 0
			|| strcmp(Name, "Scale") == 0;
		};

	// Pass 1: Transform 프로퍼티 먼저 (Root가 아닐 때만)
	if (!bIsRoot)
	{
		for (auto& Prop : Props)
		{
			if (IsTransformProp(Prop.Name))
			{
				RenderPropertyWidget(Prop);
				if (bAbortPropertyRender)
				{
					break;
				}
			}
		}
		if (!bAbortPropertyRender)
		{
			ImGui::Separator();
		}
	}

	// Pass 2: 나머지 프로퍼티
	if (!bAbortPropertyRender) for (auto& Prop : Props)
	{
		if (IsTransformProp(Prop.Name))
			continue;
		RenderPropertyWidget(Prop);
		if (bAbortPropertyRender)
		{
			break;
		}
	}

	// 프로퍼티 직접 편집 후 월드 행렬 갱신
	if (SelectedComponent->IsA<USceneComponent>())
	{
		static_cast<USceneComponent*>(SelectedComponent)->MarkTransformDirty();
	}
}

void FEditorPropertyWidget::RenderPropertyWidget(FPropertyDescriptor& Prop)
{
	bool bChanged = false;

	switch (Prop.Type)
	{
	case EPropertyType::Bool:
	{
		bool* Val = static_cast<bool*>(Prop.ValuePtr);
		bChanged = ImGui::Checkbox(Prop.Name, Val);
		break;
	}
	case EPropertyType::Int:
	{
		int32* Val = static_cast<int32*>(Prop.ValuePtr);
		bChanged = ImGui::DragInt(Prop.Name, Val);
		break;
	}
	case EPropertyType::Float:
	{
		float* Val = static_cast<float*>(Prop.ValuePtr);
		if (Prop.Min != 0.0f || Prop.Max != 0.0f)
			bChanged = ImGui::DragFloat(Prop.Name, Val, Prop.Speed, Prop.Min, Prop.Max);
		else
			bChanged = ImGui::DragFloat(Prop.Name, Val, Prop.Speed);
		break;
	}
	case EPropertyType::Vec3:
	{
		float* Val = static_cast<float*>(Prop.ValuePtr);
		bChanged = ImGui::DragFloat3(Prop.Name, Val, Prop.Speed);
		break;
	}
	case EPropertyType::Vec4:
	{
		float* Val = static_cast<float*>(Prop.ValuePtr);
		bChanged = ImGui::ColorEdit4(Prop.Name, Val);
		break;
	}
	case EPropertyType::String:
	{
		FString* Val = static_cast<FString*>(Prop.ValuePtr);
		char Buf[256];
		strncpy_s(Buf, sizeof(Buf), Val->c_str(), _TRUNCATE);
		if (ImGui::InputText(Prop.Name, Buf, sizeof(Buf)))
		{
			*Val = Buf;
			bChanged = true;
		}
		break;
	}
	case EPropertyType::Object:
	{
		UObject** Value = static_cast<UObject**>(Prop.ValuePtr);
		UObject* CurrentObject = Value ? *Value : nullptr;

		if (Prop.ObjectClass == &UStaticMesh::s_TypeInfo)
		{
			UStaticMesh* CurrentMesh = CurrentObject ? CurrentObject->Cast<UStaticMesh>() : nullptr;
			FString CurrentLabel = CurrentMesh ? CurrentMesh->GetDisplayName() : FString("None");

			ImGui::PushID(Prop.Name);
			if (ImGui::BeginCombo("##StaticMeshPicker", CurrentLabel.c_str()))
			{
				const TArray<FString>& AssetPaths = GetStaticMeshAssetOptions();
				const bool bNoneSelected = (CurrentMesh == nullptr);
				if (ImGui::Selectable("None##NullStaticMesh", bNoneSelected))
				{
					if (Value)
					{
						*Value = nullptr;
						bChanged = true;
					}
				}
				if (bNoneSelected)
				{
					ImGui::SetItemDefaultFocus();
				}

				for (const FString& AssetPath : AssetPaths)
				{
					const bool bSelected =
						CurrentMesh &&
						CurrentMesh->GetAssetPathFileName() == AssetPath;

					FString CandidateLabel = GetStaticMeshAssetDisplayName(AssetPath);
					if (CandidateLabel.empty())
					{
						CandidateLabel = AssetPath;
					}

					FString CandidateIdLabel = CandidateLabel + "##" + AssetPath;
					if (ImGui::Selectable(CandidateIdLabel.c_str(), bSelected))
					{
						if (Value)
						{
							if (UStaticMesh* Candidate = FStaticMeshManager::Get().LoadStaticMeshObject(AssetPath))
							{
								*Value = Candidate;
								bChanged = true;
							}
						}
					}

					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}
			ImGui::SameLine();
			ImGui::TextUnformatted(Prop.Name);
			ImGui::PopID();
			break;
		}

		int32 ElementIndex = -1;
		UStaticMeshComponent* StaticMeshComponent = nullptr;
		TArray<UMaterialInterface*> MaterialCandidates;
		UMaterialInterface* CurrentMaterial = CurrentObject ? CurrentObject->Cast<UMaterialInterface>() : nullptr;
		if (SelectedComponent && SelectedComponent->IsA<UStaticMeshComponent>())
		{
			StaticMeshComponent = static_cast<UStaticMeshComponent*>(SelectedComponent);
			ElementIndex = ParseElementIndex(Prop.Name);
			if (ElementIndex >= 0)
			{
				MaterialCandidates = GetRelevantMaterialOptions(
					StaticMeshComponent,
					ElementIndex,
					CurrentObject ? CurrentObject->Cast<UMaterialInterface>() : nullptr);
			}
		}

		UMaterialInterface* PreviewMaterial = ResolveSectionPreviewMaterial(StaticMeshComponent, ElementIndex, CurrentMaterial);
		const bool bUsingDefaultPreview = (CurrentMaterial == nullptr && PreviewMaterial != nullptr);
		FString CurrentLabel = CurrentMaterial
			? GetMaterialDisplayLabel(CurrentMaterial)
			: (CurrentObject ? CurrentObject->GetFName().ToString() : FString("None"));
		if (CurrentLabel.empty() && CurrentObject)
		{
			CurrentLabel = CurrentObject->GetTypeInfo()->name;
		}
		if (bUsingDefaultPreview)
		{
			CurrentLabel = GetMaterialDisplayLabel(PreviewMaterial) + " (default)";
		}

		ImGui::PushID(Prop.Name);
		if (ImGui::BeginCombo("##ObjectPicker", CurrentLabel.c_str()))
		{
			const bool bNoneSelected = (CurrentObject == nullptr);
			if (ImGui::Selectable("None##NullObject", bNoneSelected))
			{
				if (Value)
				{
					*Value = nullptr;
					bChanged = true;
				}
			}
			if (bNoneSelected)
			{
				ImGui::SetItemDefaultFocus();
			}

			const bool bUseFilteredMaterialList =
				Prop.ObjectClass == &UMaterialInterface::s_TypeInfo && !MaterialCandidates.empty();

			if (bUseFilteredMaterialList)
			{
				for (UMaterialInterface* Candidate : MaterialCandidates)
				{
					if (!Candidate)
					{
						continue;
					}

					FString CandidateLabel = GetMaterialDisplayLabel(Candidate);

					const bool bSelected = (Candidate == CurrentObject);
					const FString CandidateUniqueId = std::to_string(reinterpret_cast<uintptr_t>(Candidate));
					FString CandidateIdLabel = CandidateLabel + "##" + CandidateUniqueId;

					ImGui::PushID(CandidateUniqueId.c_str());
					RenderMaterialTexturePreview(Candidate, "Preview", 40.0f);
					if (ResolveMaterialTextureSRV(Candidate))
					{
						ImGui::SameLine();
					}
					if (ImGui::Selectable(CandidateIdLabel.c_str(), bSelected, 0, ImVec2(0.0f, 40.0f)))
					{
						if (Value)
						{
							*Value = Candidate;
							bChanged = true;
						}
					}

					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
					ImGui::PopID();
				}
			}
			else for (FObjectIterator It(Prop.ObjectClass); It; ++It)
			{
				UObject* Candidate = *It;
				if (!Candidate)
				{
					continue;
				}

				UMaterialInterface* CandidateMaterial = Candidate->Cast<UMaterialInterface>();
				FString CandidateLabel = CandidateMaterial
					? GetMaterialDisplayLabel(CandidateMaterial)
					: Candidate->GetFName().ToString();
				if (CandidateLabel.empty())
				{
					CandidateLabel = Candidate->GetTypeInfo()->name;
				}

				const bool bSelected = (Candidate == CurrentObject);
				FString CandidateIdLabel = CandidateLabel + "##" + std::to_string(reinterpret_cast<uintptr_t>(Candidate));
				if (ImGui::Selectable(CandidateIdLabel.c_str(), bSelected))
				{
					if (Value)
					{
						*Value = Candidate;
						bChanged = true;
					}
				}

				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::TextUnformatted(Prop.Name);

		if (PreviewMaterial)
		{
			const FString PreviewTexturePath = ResolveMaterialTexturePath(PreviewMaterial);
			const FString PreviewTextureLabel = GetTextureDisplayName(PreviewTexturePath);
			if (!PreviewTextureLabel.empty())
			{
				if (bUsingDefaultPreview)
				{
					ImGui::TextDisabled("Texture: %s (default)", PreviewTextureLabel.c_str());
				}
				else
				{
					ImGui::TextDisabled("Texture: %s", PreviewTextureLabel.c_str());
				}

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", PreviewTexturePath.c_str());
				}
			}

			RenderMaterialTexturePreview(PreviewMaterial, "CurrentTexturePreview", 72.0f);
		}
		ImGui::PopID();
		break;
	}
	case EPropertyType::Name:
	{
		FName* Val = static_cast<FName*>(Prop.ValuePtr);
		FString Current = Val->ToString();

		// 리소스 키와 매칭되는 프로퍼티면 콤보 박스로 렌더링
		TArray<FString> Names;
		if (strcmp(Prop.Name, "Font") == 0)
			Names = FResourceManager::Get().GetFontNames();
		else if (strcmp(Prop.Name, "Particle") == 0)
			Names = FResourceManager::Get().GetParticleNames();

		if (!Names.empty())
		{
			if (ImGui::BeginCombo(Prop.Name, Current.c_str()))
			{
				for (const auto& Name : Names)
				{
					bool bSelected = (Current == Name);
					if (ImGui::Selectable(Name.c_str(), bSelected))
					{
						*Val = FName(Name);
						bChanged = true;
					}
					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		else
		{
			char Buf[256];
			strncpy_s(Buf, sizeof(Buf), Current.c_str(), _TRUNCATE);
			if (ImGui::InputText(Prop.Name, Buf, sizeof(Buf)))
			{
				*Val = FName(Buf);
				bChanged = true;
			}
		}
		break;
	}
	}

	if (bChanged && SelectedComponent)
	{
		SelectedComponent->PostEditProperty(Prop.Name);
		if (std::strcmp(Prop.Name, "StaticMesh") == 0)
		{
			bAbortPropertyRender = true;
		}
	}
}
