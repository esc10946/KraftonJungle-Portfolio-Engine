#include "Editor/UI/EditorViewportOverlayWidget.h"

#include "Editor/Settings/EditorSettings.h"
#include "ImGui/imgui.h"

void FEditorViewportOverlayWidget::Render(float DeltaTime)
{
	(void)DeltaTime;

	FEditorSettings& Settings = FEditorSettings::Get();

	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	const float Padding = 10.0f;
	ImVec2 WindowPos(Viewport->WorkPos.x + Viewport->WorkSize.x - Padding, Viewport->WorkPos.y + Padding);

	ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
	ImGui::SetNextWindowBgAlpha(0.6f);

	ImGuiWindowFlags Flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoMove;

	if (!ImGui::Begin("##ViewportOverlay", nullptr, Flags))
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button(bExpanded ? "Settings <<" : "Settings >>"))
	{
		bExpanded = !bExpanded;
	}

	if (bExpanded)
	{
		ImGui::Separator();

		// Grid Settings
		ImGui::Text("Grid");
		ImGui::SliderFloat("Spacing", &Settings.GridSpacing, 0.1f, 10.0f, "%.1f");
		ImGui::SliderInt("Half Line Count", &Settings.GridHalfLineCount, 10, 500);

		ImGui::Separator();

		// Camera Sensitivity
		ImGui::Text("Camera");
		ImGui::SliderFloat("Move Sensitivity", &Settings.CameraMoveSensitivity, 0.1f, 5.0f, "%.1f");
		ImGui::SliderFloat("Rotate Sensitivity", &Settings.CameraRotateSensitivity, 0.1f, 5.0f, "%.1f");
	}

	ImGui::End();
}
