#include "SViewport.h"
#include "SSplitter.h"
#include "SSplitterH.h"
#include "SSplitterV.h"
#include "SViewportRoot.h"
#include "SlateApplication.h"
#include "Viewport/Viewport.h"
#include "Viewport/EditorViewportClient.h"
#include "Viewport/ViewportManager.h"
#include "SlateWidgetFactory.h"
#include "ImGui/imgui.h"
#include "UI/EditorConsoleWidget.h"

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

void SViewport::OnPaint()
{
	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;

	// 구조 아쉬움
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

	ImGui::Text("Camera View");
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

	ImGui::Text("View Mode");
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

	ImGui::Text("Show Flags");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##Show Flags", "Select Flags..."))
	{
		ImGui::Checkbox("Primitives", &ShowFlags.bPrimitives);
		ImGui::Checkbox("BillboardText", &ShowFlags.bBillboardText);
		ImGui::Checkbox("Grid", &ShowFlags.bGrid);
		ImGui::Checkbox("Gizmo", &ShowFlags.bGizmo);
		ImGui::Checkbox("Bounding Volume", &ShowFlags.bBoundingVolume);

		ImGui::EndCombo();

		ViewOption.ShowFlags = ShowFlags;
	}

	ImGui::PopItemWidth();

	ImGui::Text("Change Viewport Layout");

	if (ImGui::Button("Single"))
	{
		SWidget* Root = FSlateApplication::Get().GetWindows()[0];
		SViewportRoot* ViewportRoot = FSlateApplication::Get().GetWidgetsByType<SViewportRoot*>(Root)[0];

		if (ViewportRoot)
		{
			ViewportRoot->SetVisibleCount(1);
		}
	}
	if (ImGui::Button("Multiple"))
	{
		SWidget* Root = FSlateApplication::Get().GetWindows()[0];
		SViewportRoot* ViewportRoot = FSlateApplication::Get().GetWidgetsByType<SViewportRoot*>(Root)[0];

		if (ViewportRoot)
		{
			ViewportRoot->SetVisibleCount(4);
		}
	}

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
