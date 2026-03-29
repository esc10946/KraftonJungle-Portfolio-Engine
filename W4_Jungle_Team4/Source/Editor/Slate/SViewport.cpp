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
			Client->OnViewportResized(Rect.X, Rect.Y, Rect.Width, Rect.Height);
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

	ImGui::SetNextWindowPos(ImVec2((float)Rect.X, (float)Rect.Y));
	ImGui::SetNextWindowSize(ImVec2((float)Rect.Width, (float)Rect.Height));

	ImGui::Begin(GetName().c_str(), nullptr, WindowFlags);

	ImGui::Text("View Mode");
	int CurrentMode = static_cast<int32>(Client->GetViewMode());
	ImGui::RadioButton("Lit", &CurrentMode, static_cast<int32>(EViewMode::Lit));
	ImGui::SameLine();
	ImGui::RadioButton("Unlit", &CurrentMode, static_cast<int32>(EViewMode::Unlit));
	ImGui::SameLine();
	ImGui::RadioButton("Wireframe", &CurrentMode, static_cast<int32>(EViewMode::Wireframe));
	Client->SetViewMode(static_cast<EViewMode>(CurrentMode));

	FShowFlags ShowFlags = Client->GetShowFlags();

	ImGui::Text("Show");
	ImGui::Checkbox("Primitives", &ShowFlags.bPrimitives);
	ImGui::Checkbox("BillboardText", &ShowFlags.bBillboardText);
	ImGui::Checkbox("Grid", &ShowFlags.bGrid);
	ImGui::Checkbox("Gizmo", &ShowFlags.bGizmo);
	ImGui::Checkbox("Bounding Volume", &ShowFlags.bBoundingVolume);

	Client->SetShowFlags(ShowFlags);

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
