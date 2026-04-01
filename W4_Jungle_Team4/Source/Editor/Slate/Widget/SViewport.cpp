#include "SViewport.h"
#include "SSplitter.h"
#include "SSplitterH.h"
#include "SSplitterV.h"
#include "SViewportRoot.h"
#include "SViewportOverlayStat.h"
#include "Slate/SlateApplication.h"
#include "Slate/SlateWidgetFactory.h"
#include "Viewport/Viewport.h"
#include "Viewport/EditorViewportClient.h"
#include "Viewport/ViewportManager.h"
#include "ImGui/imgui.h"
#include "UI/EditorConsoleWidget.h"
#include "Math/Utils.h"

SViewport::SViewport()
{
	OverlayStat = new SViewportOverlayStat(this);
}

void SViewport::Tick(float DeltaTime)
{
	if (Viewport)
	{
		FViewportClient* Client = Viewport->GetViewportClient();
		if (Client)
		{
			Client->Tick(DeltaTime);
		}
	}
}

bool RenderLayoutButton(const char* label, EViewportLayout Layout, ImVec2 size, bool selected)
{
	ImGui::PushID(label);

	ImVec2 p = ImGui::GetCursorScreenPos();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	ImGui::InvisibleButton(label, size);
	bool is_hovered = ImGui::IsItemHovered();
	bool is_active = ImGui::IsItemActive();
	bool is_clicked = ImGui::IsItemClicked();

	ImU32 bg_col = selected ? IM_COL32(0, 120, 215, 255) :
		(is_hovered ? IM_COL32(70, 70, 70, 255) : IM_COL32(50, 50, 50, 255));

	draw_list->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bg_col, 4.0f);

	// 4. 레이아웃 선 그리기 (예: 2분할 세로)
	float pad = 6.0f;
	ImVec2 min = { p.x + pad, p.y + pad };
	ImVec2 max = { p.x + size.x - pad, p.y + size.y - pad };
	ImU32 line_col = IM_COL32(200, 200, 200, 255);

	draw_list->AddRect(min, max, line_col); // 테두리
	
	switch (Layout)
	{
	case EViewportLayout::Single:
		break;
	case EViewportLayout::TwoVertical:
		draw_list->AddLine(ImVec2((min.x + max.x) * 0.5f, min.y),
			ImVec2((min.x + max.x) * 0.5f, max.y), line_col);
		break;
	case EViewportLayout::TwoHorizontal:
		draw_list->AddLine(ImVec2(min.x, (min.y + max.y) * 0.5f),
			ImVec2(max.x, (min.y + max.y) * 0.5f), line_col);
		break;
	case EViewportLayout::ThreeLeftMain:
		draw_list->AddLine(ImVec2((min.x + max.x) * 0.5f, min.y),
			ImVec2((min.x + max.x) * 0.5f, max.y), line_col);
		draw_list->AddLine(ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f),
			ImVec2(max.x, (min.y + max.y) * 0.5f), line_col);
		break;
	case EViewportLayout::ThreeRightMain:
		draw_list->AddLine(ImVec2((min.x + max.x) * 0.5f, min.y),
			ImVec2((min.x + max.x) * 0.5f, max.y), line_col);
		draw_list->AddLine(ImVec2(min.x, (min.y + max.y) * 0.5f),
			ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f), line_col);
		break;
	case EViewportLayout::ThreeTopMain:
		draw_list->AddLine(ImVec2(min.x, (min.y + max.y) * 0.5f),
			ImVec2(max.x, (min.y + max.y) * 0.5f), line_col);
		draw_list->AddLine(ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f),
			ImVec2((min.x + max.x) * 0.5f, max.y), line_col);
		break;
	case EViewportLayout::ThreeBottomMain:
		draw_list->AddLine(ImVec2(min.x, (min.y + max.y) * 0.5f),
			ImVec2(max.x, (min.y + max.y) * 0.5f), line_col);
		draw_list->AddLine(ImVec2((min.x + max.x) * 0.5f, min.y),
			ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f), line_col);
		break;
	case EViewportLayout::FourQuad:
		draw_list->AddLine(ImVec2(min.x, (min.y + max.y) * 0.5f),
			ImVec2(max.x, (min.y + max.y) * 0.5f), line_col);
		draw_list->AddLine(ImVec2((min.x + max.x) * 0.5f, min.y),
			ImVec2((min.x + max.x) * 0.5f, max.y), line_col);
		break;
	case EViewportLayout::FourLeftMain:
		draw_list->AddLine(ImVec2(Lerp(min.x, max.x, 0.66f), min.y),
			ImVec2(Lerp(min.x, max.x, 0.66f), max.y), line_col);
		draw_list->AddLine(ImVec2(Lerp(min.x, max.x, 0.66f), Lerp(min.y, max.y, 0.33f)),
			ImVec2(max.x, Lerp(min.y, max.y, 0.33f)), line_col);
		draw_list->AddLine(ImVec2(Lerp(min.x, max.x, 0.66f), Lerp(min.y, max.y, 0.66f)),
			ImVec2(max.x, Lerp(min.y, max.y, 0.66f)), line_col);
		break;
	case EViewportLayout::FourRightMain:
		draw_list->AddLine(ImVec2(Lerp(min.x, max.x, 0.33f), min.y),
			ImVec2(Lerp(min.x, max.x, 0.33f), max.y), line_col);
		draw_list->AddLine(ImVec2(min.x, Lerp(min.y, max.y, 0.33f)),
			ImVec2(Lerp(min.x, max.x, 0.33f), Lerp(min.y, max.y, 0.33f)), line_col);
		draw_list->AddLine(ImVec2(min.x, Lerp(min.y, max.y, 0.66f)),
			ImVec2(Lerp(min.x, max.x, 0.33f), Lerp(min.y, max.y, 0.66f)), line_col);
		break;
	case EViewportLayout::FourTopMain:
		draw_list->AddLine(ImVec2(min.x, Lerp(min.y, max.y, 0.66f)),
			ImVec2(max.x, Lerp(min.y, max.y, 0.66f)), line_col);
		draw_list->AddLine(ImVec2(Lerp(min.x, max.x, 0.33f), Lerp(min.y, max.y, 0.66f)),
			ImVec2(Lerp(min.x, max.x, 0.33f), max.y), line_col);
		draw_list->AddLine(ImVec2(Lerp(min.x, max.x, 0.66f), Lerp(min.y, max.y, 0.66f)),
			ImVec2(Lerp(min.x, max.x, 0.66f), max.y), line_col);
		break;
	case EViewportLayout::FourBottomMain:
		draw_list->AddLine(ImVec2(min.x, Lerp(min.y, max.y, 0.33f)),
			ImVec2(max.x, Lerp(min.y, max.y, 0.33f)), line_col);
		draw_list->AddLine(ImVec2(Lerp(min.x, max.x, 0.33f), min.y),
			ImVec2(Lerp(min.x, max.x, 0.33f), Lerp(min.y, max.y, 0.33f)), line_col);
		draw_list->AddLine(ImVec2(Lerp(min.x, max.x, 0.66f), min.y),
			ImVec2(Lerp(min.x, max.x, 0.66f), Lerp(min.y, max.y, 0.33f)), line_col);
		break;
	}

	ImGui::PopID();

	return is_clicked;
}

void SViewport::OnPaint()
{
	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;

	FEditorViewportClient* Client = dynamic_cast<FEditorViewportClient*>(Viewport->GetViewportClient());
	if (Client == nullptr) return;

	FViewOption ViewOption = Client->GetCurrentViewOption();
	bool bIsViewOptionChanged = false;

	ImGui::SetNextWindowPos(ImVec2((float)Rect.X, (float)Rect.Y));
	ImGui::SetNextWindowSize(ImVec2((float)Rect.Width, (float)Rect.Height));

	ImGui::Begin(GetName().c_str(), nullptr, WindowFlags);

	ImGui::PushItemWidth(120.0f);

	int32 CurrentCameraType = static_cast<int32>(ViewOption.Type);
	const char* CameraTypeNames[] = { "Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back" };
	const EViewOption CameraTypeValues[] = { EViewOption::Perspective, EViewOption::Top, EViewOption::Bottom, EViewOption::Left, EViewOption::Right, EViewOption::Front, EViewOption::Back };

	ImGui::SameLine();
	if (ImGui::BeginCombo("##Camera View", CameraTypeNames[CurrentCameraType]))
	{
		for (uint32 i = 0; i < static_cast<uint32>(EViewOption::Max); ++i)
		{
			const bool bIsSelected = (CurrentCameraType == i);
			if (ImGui::Selectable(CameraTypeNames[i], bIsSelected))
			{
				CurrentCameraType = i;
			}
			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
		
		if (ViewOption.Type != static_cast<EViewOption>(CurrentCameraType))
		{
			bIsViewOptionChanged = true;
		}
	}

	int32 CurrentMode = static_cast<int32>(ViewOption.ViewMode);
	const char* ViewModeNames[] = { "Lit", "Unlit", "Wireframe" };
	const EViewMode ViewModeValues[] = { EViewMode::Lit, EViewMode::Unlit, EViewMode::Wireframe };

	ImGui::SameLine();
	if (ImGui::BeginCombo("##View Mode", ViewModeNames[CurrentMode]))
	{
		for (uint32 i = 0; i < static_cast<uint32>(EViewMode::Count); ++i)
		{
			const bool bIsSelected = (CurrentMode == i);
			if (ImGui::Selectable(ViewModeNames[i], bIsSelected))
			{
				CurrentMode = i;
			}

			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
		
		ViewOption.ViewMode = static_cast<EViewMode>(CurrentMode);
		bIsViewOptionChanged = true;
	}

	ImGui::SameLine();

	FShowFlags ShowFlags = ViewOption.ShowFlags;
	if (ImGui::BeginCombo("##Show", "Show", ImGuiComboFlags_HeightLarge))
	{
		if (ImGui::TreeNode("Viewport Stats")) {
			ImGui::MenuItem("FPS", nullptr, &bShowFPS);
			ImGui::MenuItem("Memory", nullptr, &bShowMemory);
			ImGui::TreePop();
		}

		ImGui::Separator();

		ImGui::MenuItem("Primitives", nullptr, &ShowFlags.bPrimitives);
		ImGui::MenuItem("BillboardText", nullptr, &ShowFlags.bBillboardText);
		ImGui::MenuItem("Grid", nullptr, &ShowFlags.bGrid);
		ImGui::MenuItem("Gizmo", nullptr, &ShowFlags.bGizmo);
		ImGui::MenuItem("Bounding Volume", nullptr, &ShowFlags.bBoundingVolume);

		ImGui::EndCombo();

		ViewOption.ShowFlags = ShowFlags;
	}

	SWidget* Root = FSlateApplication::Get().GetWindows()[0];
	SViewportRoot* ViewportRoot = FSlateApplication::Get().GetWidgetsByType<SViewportRoot*>(Root)[0];

	ImGui::SameLine();

	if (ImGui::BeginCombo("##Layout", "Layout"))
	{
		ImGui::SeparatorText("1 Panel");
		if (RenderLayoutButton("##LayoutSingle", EViewportLayout::Single, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::Single))
		{
			ViewportRoot->SetLayout(EViewportLayout::Single);
		}
		
		ImGui::SeparatorText("2 Panels");
		if (RenderLayoutButton("##LayoutTwoH", EViewportLayout::TwoHorizontal, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::TwoHorizontal))
		{
			ViewportRoot->SetLayout(EViewportLayout::TwoHorizontal);
		}
		ImGui::SameLine();
		if (RenderLayoutButton("##LayoutTwoV", EViewportLayout::TwoVertical, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::TwoVertical))
		{
			ViewportRoot->SetLayout(EViewportLayout::TwoVertical);
		}

		ImGui::SeparatorText("3 Panels");
		if (RenderLayoutButton("##LayoutThreeTopMain", EViewportLayout::ThreeTopMain, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::ThreeTopMain))
		{
			ViewportRoot->SetLayout(EViewportLayout::ThreeTopMain);
		}
		ImGui::SameLine();
		if (RenderLayoutButton("##LayoutThreeBottomMain", EViewportLayout::ThreeBottomMain, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::ThreeBottomMain))
		{
			ViewportRoot->SetLayout(EViewportLayout::ThreeBottomMain);
		}
		ImGui::SameLine();
		if (RenderLayoutButton("##LayoutThreeLeftMain", EViewportLayout::ThreeLeftMain, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::ThreeLeftMain))
		{
			ViewportRoot->SetLayout(EViewportLayout::ThreeLeftMain);
		}
		ImGui::SameLine();
		if (RenderLayoutButton("##LayoutThreeRightMain", EViewportLayout::ThreeRightMain, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::ThreeRightMain))
		{
			ViewportRoot->SetLayout(EViewportLayout::ThreeRightMain);
		}

		ImGui::SeparatorText("4 Panels");
		if (RenderLayoutButton("##LayoutFourQuad", EViewportLayout::FourQuad, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::FourQuad))
		{
			ViewportRoot->SetLayout(EViewportLayout::FourQuad);
		}
		ImGui::SameLine();
		if (RenderLayoutButton("##LayoutFourTopMain", EViewportLayout::FourTopMain, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::FourTopMain))
		{
			ViewportRoot->SetLayout(EViewportLayout::FourTopMain);
		}
		ImGui::SameLine();
		if (RenderLayoutButton("##LayoutFourBottomMain", EViewportLayout::FourBottomMain, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::FourBottomMain))
		{
			ViewportRoot->SetLayout(EViewportLayout::FourBottomMain);
		}
		ImGui::SameLine();
		if (RenderLayoutButton("##LayoutFourLeftMain", EViewportLayout::FourLeftMain, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::FourLeftMain))
		{
			ViewportRoot->SetLayout(EViewportLayout::FourLeftMain);
		}
		ImGui::SameLine();
		if (RenderLayoutButton("##LayoutFourRightMain", EViewportLayout::FourRightMain, ImVec2(50, 50), ViewportRoot->GetLayout() == EViewportLayout::FourRightMain))
		{
			ViewportRoot->SetLayout(EViewportLayout::FourRightMain);
		}

		ImGui::EndCombo();
	}

	ImGui::PopItemWidth();

	if (bShowFPS) OverlayStat->DrawFPSStats();
	if (bShowMemory) OverlayStat->DrawMemoryStats();

	ImGui::End();

	Client->SetViewOption(ViewOption.Type, ViewOption);

	if (bIsViewOptionChanged)
	{
		Client->SetCurrentViewOption(static_cast<EViewOption>(CurrentCameraType));
	}
}

void SViewport::OnResize(const FRect& NewRect)
{
	SWidget::OnResize(NewRect);
	if (Viewport)
	{
		FViewportClient* Client = Viewport->GetViewportClient();
		if (Client)
		{
			Client->OnViewportResized(NewRect.X, NewRect.Y, NewRect.Width, NewRect.Height);
		}
	}
}

FReply SViewport::OnMouseButtonDown(float X, float Y, int32 Button)
{
	FReply Reply = SWidget::OnMouseButtonUp(X, Y, Button);
	if (Reply.IsHandled())
	{
		return Reply;
	}

	if (Viewport)
	{
		if (Viewport->GetViewportClient())
		{
			FViewportManager::Get().SetFocusedViewportWidget(this);

			float RelativeX = X - Rect.X;
			float RelativeY = Y - Rect.Y;

			Viewport->GetViewportClient()->OnMouseDown(RelativeX, RelativeY, Button);
			return FReply::Handled().CaptureMouse(this);
		}
	}

	return FReply::Unhandled();
}

FReply SViewport::OnMouseButtonUp(float X, float Y, int32 Button)
{
	FReply Reply = SWidget::OnMouseButtonUp(X, Y, Button);
	if (Reply.IsHandled())
	{
		return Reply;
	}

	if (Viewport)
	{
		if (Viewport->GetViewportClient())
		{
			float RelativeX = X - Rect.X;
			float RelativeY = Y - Rect.Y;

			Viewport->GetViewportClient()->OnMouseUp(RelativeX, RelativeY, Button);
		}
		return FReply::Handled().CaptureMouse(nullptr);
	}

	return FReply::Unhandled();
}

FReply SViewport::OnMouseMove(float X, float Y)
{
	FReply Reply = SWidget::OnMouseMove(X, Y);
	if (Reply.IsHandled())
	{
		return Reply;
	}

	if (Viewport)
	{
		if (Viewport->GetViewportClient())
		{
			float RelativeX = X - Rect.X;
			float RelativeY = Y - Rect.Y;

			Viewport->GetViewportClient()->OnMouseMove(RelativeX, RelativeY);
		}
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

FReply SViewport::OnMouseWheel(float Delta)
{
	FReply Reply = SWidget::OnMouseWheel(Delta);
	if (Reply.IsHandled())
	{
		return Reply;
	}
	if (Viewport)
	{
		if (Viewport->GetViewportClient())
		{
			Viewport->GetViewportClient()->OnMouseWheel(Delta);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SViewport::OnKeyDown(int32 Key)
{
	SWidget::OnKeyDown(Key);
	if (Viewport && Viewport->GetViewportClient())
	{
		Viewport->GetViewportClient()->OnKeyDown(Key);
	}
}

void SViewport::OnKeyUp(int32 Key)
{
	SWidget::OnKeyUp(Key);
	if (Viewport && Viewport->GetViewportClient())
	{
		Viewport->GetViewportClient()->OnKeyUp(Key);
	}
}
