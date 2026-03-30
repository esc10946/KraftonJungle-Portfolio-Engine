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
#include "Object/EngineStatics.h"

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

	if (bShowFPS) DrawFPSStats();
	if (bShowMemory) DrawMemoryStats();

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

void SViewport::DrawFPSStats()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

	float PAD = 10.0f;
	ImVec2 window_pos = ImVec2((float)Rect.X + (float)Rect.Width - PAD, (float)Rect.Y + PAD);
	ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

	if (ImGui::Begin(("##" + GetName()).c_str(), nullptr, window_flags))
	{
		ImGui::Text("%.2f FPS", io.Framerate);
		ImGui::Text("%.2f ms", 1000.0f / io.Framerate);

		ImGui::End();
	}
}

void SViewport::DrawMemoryStats()
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

	ImVec2 window_pos = ImVec2((float)Rect.X + (float)Rect.Width * 0.5f, (float)Rect.Y + 60.0f);
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
	ImGui::SetNextWindowBgAlpha(0.4f);

	if (ImGui::Begin(("##Memory Stats_" + GetName()).c_str(), nullptr, window_flags)) {
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Memory Usage"); // 언리얼 특유의 초록색 글씨
		ImGui::Separator();
		ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

		if (ImGui::BeginTable("MemoryCounters", 4, flags)) {
			ImGui::TableSetupColumn("Memory Counters", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("UsedMax", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Mem%", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("Pool", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableHeadersRow();

			const char* CategoryNames[] = { "Streaming Texture Pool", "StaticMesh Total Memory", "VertexShader Memory" };

			for (int i = 0; i < 3; i++) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextColored(ImVec4(0, 1, 0, 1), CategoryNames[i]); // 언리얼 특유의 초록색 글씨

				ImGui::TableSetColumnIndex(1);
				float mb = EngineStatics::TotalAllocationBytes / (1024.0f * 1024.0f);
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%.2f MB", mb);

				ImGui::TableSetColumnIndex(2);
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "100%"); // 필요 시 계산

				ImGui::TableSetColumnIndex(3);
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Physical");
			}
			ImGui::EndTable();
		}

		ImGui::End();
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
