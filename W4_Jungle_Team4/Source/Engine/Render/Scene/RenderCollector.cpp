#include "RenderCollector.h"

#include "GameFramework/World.h"
#include "GameFramework/AActor.h"
#include "Component/PrimitiveComponent.h"
#include "Component/GizmoComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/TextRenderComponent.h"
#include "Component/SubUVComponent.h"
#include "Asset/Material/MaterialObject.h"
#include "Asset/StaticMesh/StaticMeshObject.h"
#include "Asset/Texture/TextureObject.h"
#include "Core/ResourceManager.h"
#include "Slate/SlateApplication.h"

void FRenderCollector::Release()
{
	for (auto& Pair : StaticMeshBufferMap)
	{
		Pair.second.Release();
	}

	StaticMeshBufferMap.clear();
	StaticMeshMap.clear();
	StaticMeshRevisionMap.clear();
	MeshBufferManager.Release();

	Device = nullptr;
}

void FRenderCollector::CollectWorld(UWorld* World, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus)
{
	if (!World) return;

	for (AActor* Actor : World->GetActors())
	{
		if (!Actor) continue;
		CollectFromActor(Actor, ShowFlags, ViewMode, RenderBus);
	}
}

void FRenderCollector::CollectSelection(const TArray<AActor*>& SelectedActors, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus)
{
	for (AActor* Actor : SelectedActors)
	{
		CollectFromSelectedActor(Actor, ShowFlags, ViewMode, RenderBus);
	}
}

void FRenderCollector::CollectGrid(float GridSpacing, int32 GridHalfLineCount, FRenderBus& RenderBus)
{
	FRenderCommand Cmd = {};
	Cmd.Type = ERenderCommandType::Grid;
	Cmd.Constants.Grid.GridSpacing = GridSpacing;
	Cmd.Constants.Grid.GridHalfLineCount = GridHalfLineCount;
	RenderBus.AddCommand(ERenderPass::Grid, Cmd);
}

void FRenderCollector::CollectGizmo(UGizmoComponent* Gizmo, const FShowFlags& ShowFlags, FRenderBus& RenderBus)
{
	if (ShowFlags.bGizmo == false) return;
	if (!Gizmo || !Gizmo->IsVisible()) return;

	FMeshBuffer* GizmoMesh = &MeshBufferManager.GetMeshBuffer(Gizmo->GetPrimitiveType());
	FMatrix WorldMatrix = Gizmo->GetWorldMatrix();
	bool bHolding = Gizmo->IsHolding();
	int32 SelectedAxis = Gizmo->GetSelectedAxis();

	auto CreateGizmoCmd = [&](bool bInner) {
		FRenderCommand Cmd = {};
		Cmd.Type = ERenderCommandType::Gizmo;
		Cmd.MeshBuffer = GizmoMesh;
		Cmd.PerObjectConstants = FPerObjectConstants{ WorldMatrix };

		if (bInner)
		{
			Cmd.DepthStencilState = EDepthStencilState::GizmoInside;
			Cmd.BlendState = EBlendState::AlphaBlend;
		}
		else
		{
			Cmd.DepthStencilState = EDepthStencilState::GizmoOutside;
			Cmd.BlendState = EBlendState::Opaque;
		}
		Cmd.Constants.Gizmo.ColorTint = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
		Cmd.Constants.Gizmo.bIsInnerGizmo = bInner ? 1 : 0;
		Cmd.Constants.Gizmo.bClicking = bHolding ? 1 : 0;
		Cmd.Constants.Gizmo.SelectedAxis = SelectedAxis >= 0 ? (uint32)SelectedAxis : 0xffffffffu;
		Cmd.Constants.Gizmo.HoveredAxisOpacity = 0.3f;
		return Cmd;
		};

	RenderBus.AddCommand(ERenderPass::DepthLess, CreateGizmoCmd(false));

	if (!bHolding)
	{
		RenderBus.AddCommand(ERenderPass::DepthLess, CreateGizmoCmd(true));
	}
}

void FRenderCollector::CollectFromSlate(FRenderBus& RenderBus)
{
	TArray<SWindow*> Windows = FSlateApplication::Get().GetWindows();

	for (SWindow* Window : Windows)
	{
		if (!Window) continue;

		CollectFromSlateWidget(Window, RenderBus);
	}
}

void FRenderCollector::CollectFromActor(AActor* Actor, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus)
{
	if (!Actor->IsVisible()) return;

	for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
	{
		CollectFromComponent(Primitive, ShowFlags, ViewMode, RenderBus);
	}
}

void FRenderCollector::CollectFromSelectedActor(AActor* Actor, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus)
{
	if (!Actor->IsVisible()) return;

	for (UPrimitiveComponent* primitiveComponent : Actor->GetPrimitiveComponents())
	{
		if (!primitiveComponent->IsVisible()) continue;

		// AABB는 선택 디버그 표시이므로 outline 지원 여부와 분리해 토글로만 제어한다.
		CollectAABBCommand(primitiveComponent, ShowFlags, RenderBus);

		FRenderCommand BaseCmd{};
		if (!ConfigureSelectionBaseCommand(primitiveComponent, BaseCmd)) continue;

		FVector WorldScale = primitiveComponent->GetWorldScale();

		if (primitiveComponent->GetPrimitiveType() == EPrimitiveType::EPT_Text)
		{
			UTextRenderComponent* TextComp = static_cast<UTextRenderComponent*>(primitiveComponent);
			const FFontResource* Font = TextComp->GetFont();
			if (!Font || !Font->IsLoaded()) continue;
			const FString& Text = TextComp->GetText();
			if (Text.empty()) continue;

			FMatrix outlineMatrix = TextComp->CalculateOutlineMatrix();
			WorldScale = outlineMatrix.GetScale();

			FRenderCommand TextCmd = BaseCmd;
			BaseCmd.PerObjectConstants.Model = outlineMatrix;

			if (ShowFlags.bBillboardText)
			{
				TextCmd.PerObjectConstants = FPerObjectConstants{ primitiveComponent->GetWorldMatrix() };
				TextCmd.Type = ERenderCommandType::Font;
				TextCmd.PerObjectConstants.Color = TextComp->GetColor();
				TextCmd.Constants.Font.Text = &Text;
				TextCmd.Constants.Font.Font = Font;
				TextCmd.Constants.Font.Scale = TextComp->GetFontSize();
				TextCmd.BlendState = EBlendState::AlphaBlend;
				TextCmd.DepthStencilState = EDepthStencilState::Default;
				RenderBus.AddCommand(ERenderPass::Font, TextCmd);
			}
		}

		if (!primitiveComponent->SupportsOutline()) continue;


		// StencilBuffer Mask
		FRenderCommand MaskCmd = BaseCmd;
		MaskCmd.Type = ERenderCommandType::SelectionOutline;
		MaskCmd.DepthStencilState = EDepthStencilState::StencilWrite; //스텐실 버퍼만 작성하는 타입
		MaskCmd.BlendState = EBlendState::NoColor;
		MaskCmd.Constants.Outline.OutlineOffset = 0.0f;
		RenderBus.AddCommand(ERenderPass::StencilMask, MaskCmd);

		// Outline
		FRenderCommand OutlineCmd = BaseCmd;
		OutlineCmd.Type = ERenderCommandType::SelectionOutline;
		OutlineCmd.DepthStencilState = EDepthStencilState::StencilOutline;
		OutlineCmd.Constants.Outline.OutlineColor = FVector4(1.0f, 0.5f, 0.0f, 1.0f); // RGBA
		OutlineCmd.Constants.Outline.OutlineInvScale = FVector(1.0f / WorldScale.X,
			1.0f / WorldScale.Y, 1.0f / WorldScale.Z);
		OutlineCmd.Constants.Outline.OutlineOffset = 0.03f;
		if (ViewMode == EViewMode::Wireframe)
		{
			OutlineCmd.PerObjectConstants.Color = FColor(255, 153, 0, 255).ToVector4();
		}

		EPrimitiveType PrimType = primitiveComponent->GetPrimitiveType();
		OutlineCmd.Constants.Outline.PrimitiveType = (PrimType == EPrimitiveType::EPT_Plane ||
			PrimType == EPrimitiveType::EPT_SubUV ||
			PrimType == EPrimitiveType::EPT_Text) ? 0u : 1u;
		RenderBus.AddCommand(ERenderPass::Outline, OutlineCmd);
	}
}

bool FRenderCollector::ConfigureSelectionBaseCommand(UPrimitiveComponent* PrimitiveComponent, FRenderCommand& OutBaseCmd)
{
	if (!PrimitiveComponent)
	{
		return false;
	}

	OutBaseCmd.PerObjectConstants = FPerObjectConstants{ PrimitiveComponent->GetWorldMatrix() };

	if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::EPT_StaticMesh)
	{
		UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(PrimitiveComponent);
		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
		OutBaseCmd.TextureMeshBuffer = GetOrCreateStaticMeshBuffer(StaticMeshComponent);
		if (!StaticMesh || !OutBaseCmd.TextureMeshBuffer || !OutBaseCmd.TextureMeshBuffer->IsValid())
		{
			return false;
		}

		OutBaseCmd.MeshRenderData.FirstIndex = 0;
		OutBaseCmd.MeshRenderData.IndexCount = static_cast<uint32>(StaticMesh->GetIndices().size());
		return true;
	}

	OutBaseCmd.MeshBuffer = &MeshBufferManager.GetMeshBuffer(PrimitiveComponent->GetPrimitiveType());
	return OutBaseCmd.MeshBuffer && OutBaseCmd.MeshBuffer->IsValid();
}

void FRenderCollector::CollectFromComponent(UPrimitiveComponent* Primitive, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus)
{
	if (!Primitive->IsVisible()) return;

	EPrimitiveType PrimType = Primitive->GetPrimitiveType();

	switch (PrimType)
	{
	case EPrimitiveType::EPT_Cube:
	case EPrimitiveType::EPT_Sphere:
	case EPrimitiveType::EPT_Plane:
	{
		if (!ShowFlags.bPrimitives) return;
		FRenderCommand Cmd = {};
		Cmd.PerObjectConstants = FPerObjectConstants{ Primitive->GetWorldMatrix(), Primitive->GetMaterialColor() };
		Cmd.Type = ERenderCommandType::Primitive;
		Cmd.MeshBuffer = &MeshBufferManager.GetMeshBuffer(PrimType);
		Cmd.DepthStencilState = EDepthStencilState::Default;
		RenderBus.AddCommand(ERenderPass::Opaque, Cmd);
		break;
	}

	case EPrimitiveType::EPT_SubUV:
	{
		USubUVComponent* SubUVComp = static_cast<USubUVComponent*>(Primitive);
		const FParticleResource* Particle = SubUVComp->GetParticle();
		if (!Particle || !Particle->IsLoaded()) return;

		FRenderCommand Cmd = {};
		Cmd.PerObjectConstants = FPerObjectConstants{ Primitive->GetWorldMatrix(), FColor::White().ToVector4() };
		Cmd.Type = ERenderCommandType::SubUV;
		Cmd.Constants.SubUV.Particle = Particle;
		Cmd.Constants.SubUV.FrameIndex = SubUVComp->GetFrameIndex();
		Cmd.Constants.SubUV.Width = SubUVComp->GetWidth();
		Cmd.Constants.SubUV.Height = SubUVComp->GetHeight();
		Cmd.BlendState = EBlendState::AlphaBlend;
		Cmd.DepthStencilState = EDepthStencilState::Default;
		RenderBus.AddCommand(ERenderPass::SubUV, Cmd);
		break;
	}

	case EPrimitiveType::EPT_StaticMesh:
	{
		if (!ShowFlags.bPrimitives) return;

		UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(Primitive);
		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
		FTextureMeshBuffer* StaticMeshBuffer = GetOrCreateStaticMeshBuffer(StaticMeshComponent);
		if (!StaticMesh || !StaticMeshBuffer || !StaticMeshBuffer->IsValid())
		{
			return;
		}

		const TArray<FStaticMeshSection>& Sections = StaticMesh->GetSections();
		for (size_t SectionIndex = 0; SectionIndex < Sections.size(); ++SectionIndex)
		{
			const FStaticMeshSection& Section = Sections[SectionIndex];
			FRenderCommand Cmd = {};
			Cmd.Type = ERenderCommandType::StaticMesh;
			Cmd.TextureMeshBuffer = StaticMeshBuffer;
			Cmd.PerObjectConstants = FPerObjectConstants{ Primitive->GetWorldMatrix(), Primitive->GetMaterialColor() };
			Cmd.DepthStencilState = EDepthStencilState::Default;
			Cmd.BlendState = EBlendState::Opaque;

			Cmd.MeshRenderData.FirstIndex = Section.FirstIndex;
			Cmd.MeshRenderData.IndexCount = Section.IndexCount;

			UMaterialInterface* Material = StaticMeshComponent->GetMaterial(static_cast<int32>(SectionIndex));
			FResolvedMaterialParameters MaterialParameters;
			Cmd.MeshRenderData.Material = Material;
			if (Material)
			{
				Material->ResolveParameters(MaterialParameters);
				Cmd.MeshRenderData.Shader = MaterialParameters.Shader;
				Cmd.PerObjectConstants.Color = MaterialParameters.BaseColor;
				Cmd.Constants.StaticMesh.bUseAtlas = MaterialParameters.bUseAtlas ? 1u : 0u;
				Cmd.Constants.StaticMesh.bSourceUVsUseAtlasTiles =
					(MaterialParameters.bUseAtlas && StaticMesh->SectionUsesEmbeddedAtlasUVs(static_cast<int32>(SectionIndex))) ? 1u : 0u;
				Cmd.Constants.StaticMesh.SourceAtlasTileScale = 64.0f;
				Cmd.Constants.StaticMesh.TopUVRect = MaterialParameters.TopUVRect;
				Cmd.Constants.StaticMesh.SideUVRect = MaterialParameters.SideUVRect;
				Cmd.Constants.StaticMesh.BottomUVRect = MaterialParameters.BottomUVRect;
				if (UTexture2D* Texture = MaterialParameters.DiffuseTexture)
				{
					Cmd.MeshRenderData.DiffuseSRV = Texture->GetOrCreateShaderResourceView(Device);
				}
				Cmd.Constants.StaticMesh.StaticMeshUseTexture = (Cmd.MeshRenderData.DiffuseSRV != nullptr) ? 1u : 0u;
			}

			if (StaticMeshComponent->bUseUVScroll) {
				Cmd.Constants.StaticMesh.UVScrollSpeed[0] = StaticMeshComponent->UVScrollSpeed.X;
				Cmd.Constants.StaticMesh.UVScrollSpeed[1] = StaticMeshComponent->UVScrollSpeed.Y;
			}
			Cmd.Constants.StaticMesh.UVTiling[0] = 1.0f;
			Cmd.Constants.StaticMesh.UVTiling[1] = 1.0f;
			Cmd.Constants.StaticMesh.UVOffset[0] = 0.0f;
			Cmd.Constants.StaticMesh.UVOffset[1] = 0.0f;

			RenderBus.AddCommand(ERenderPass::Opaque, Cmd);
		}

		break;
	}

	default:
		return;
	}
}

void FRenderCollector::CollectAABBCommand(UPrimitiveComponent* PrimitiveComponent, const FShowFlags& ShowFlags, FRenderBus& RenderBus)
{
	if (!ShowFlags.bBoundingVolume) return;

	FRenderCommand AABBCmd = {};
	AABBCmd.Type = ERenderCommandType::DebugBox;

	FBoundingBox Box = PrimitiveComponent->GetWorldBoundingBox();

	// 이전에 정의한 union 구조체의 AABB 영역에 데이터를 채웁니다.
	AABBCmd.Constants.AABB.Min = Box.Min;
	AABBCmd.Constants.AABB.Max = Box.Max;
	AABBCmd.Constants.AABB.Color = FColor::White();

	// 렌더러가 마지막에 몰아서 그릴 수 있게 특정 패스(예: Editor/Overlay)에 푸시합니다.
	RenderBus.AddCommand(ERenderPass::Editor, AABBCmd);
}

void FRenderCollector::CollectFromSlateWidget(SWidget* Widget, FRenderBus& RenderBus)
{
	for (SWidget* Widget : Widget->GetChildren())
	{
		if (Widget->IsRenderable())
		{
			FRect RenderRect = Widget->GetRenderRect();

			FRenderCommand Cmd = {};
			Cmd.Type = ERenderCommandType::Slate;
			Cmd.MeshBuffer = &MeshBufferManager.GetMeshBuffer(EPrimitiveType::EPT_Quad2D);
			Cmd.PerObjectConstants.Color = Widget->GetColor().ToVector4();
			Cmd.Constants.Slate.ScreenPosition = { (float)RenderRect.X, (float)RenderRect.Y };
			Cmd.Constants.Slate.ScreenSize = { (float)RenderRect.Width, (float)RenderRect.Height };
			Cmd.DepthStencilState = EDepthStencilState::Default;

			RenderBus.AddCommand(ERenderPass::Slate, Cmd);
		}

		CollectFromSlateWidget(static_cast<SWindow*>(Widget), RenderBus);
	}
}

FTextureMeshBuffer* FRenderCollector::GetOrCreateStaticMeshBuffer(UStaticMeshComponent* StaticMeshComponent)
{
	if (!StaticMeshComponent || !Device)
	{
		return nullptr;
	}

	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	const FTexturedMeshData* TexturedMeshData = StaticMeshComponent->GetTexturedMeshData();
	if (!StaticMesh || !TexturedMeshData)
	{
		return nullptr;
	}

	FTextureMeshBuffer& MeshBuffer = StaticMeshBufferMap[StaticMeshComponent];
	const auto It = StaticMeshMap.find(StaticMeshComponent);
	const uint32 CurrentRevision = StaticMeshComponent->GetTexturedMeshRevision();
	const auto RevisionIt = StaticMeshRevisionMap.find(StaticMeshComponent);
	const bool bRevisionChanged = (RevisionIt == StaticMeshRevisionMap.end()) || (RevisionIt->second != CurrentRevision);
	const bool bNeedsRebuild =
		(It == StaticMeshMap.end()) ||
		(It->second != StaticMesh) ||
		!MeshBuffer.IsValid() ||
		bRevisionChanged;
	if (bNeedsRebuild)
	{
		MeshBuffer.Release();
		MeshBuffer.Create(Device, *TexturedMeshData);
		StaticMeshMap[StaticMeshComponent] = StaticMesh;
		StaticMeshRevisionMap[StaticMeshComponent] = CurrentRevision;
	}

	return MeshBuffer.IsValid() ? &MeshBuffer : nullptr;
}
