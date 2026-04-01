#include "SViewportOverlayStat.h"

#include "SViewport.h"
#include "ImGui/imgui.h"
#include "Object/EngineStatics.h"

void SViewportOverlayStat::DrawFPSStats()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

	const FRect& Rect = Viewport->GetRect();

	float PAD = 10.0f;
	ImVec2 window_pos = ImVec2((float)Rect.X + (float)Rect.Width - PAD, (float)Rect.Y + PAD);
	ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

	if (Rect.Width < 200.0f || Rect.Height < 100.0f) return;

	if (ImGui::Begin(("##" + Viewport->GetName()).c_str(), nullptr, window_flags))
	{
		ImGui::Text("%.2f FPS", io.Framerate);
		ImGui::Text("%.2f ms", 1000.0f / io.Framerate);

		ImGui::End();
	}
}

void SViewportOverlayStat::DrawMemoryStats()
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

	const FRect& Rect = Viewport->GetRect();

	ImVec2 MaxSize = ImVec2((float)Rect.Width, (float)Rect.Height);
	ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), MaxSize);

	ImVec2 WindowPos = ImVec2((float)Rect.X + (float)Rect.Width * 0.5f, (float)Rect.Y + 100.0f);
	ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));

	if (Rect.Width < 200.0f || Rect.Height < 200.0f) return;

	MemoryStats[0].MemoryBytes = EngineStatics::GetMemoryUsage(EMemoryCategory::Heap) / (1024.0f * 1024.0f);
	MemoryStats[1].MemoryBytes = (float)EngineStatics::GetMemoryUsage(EMemoryCategory::HeapCount);
	MemoryStats[2].MemoryBytes = EngineStatics::GetMemoryUsage(EMemoryCategory::StaticMesh) / (1024.0f * 1024.0f);
	MemoryStats[3].MemoryBytes = EngineStatics::GetMemoryUsage(EMemoryCategory::Texture) / (1024.0f * 1024.0f);

	if (ImGui::Begin(("##Memory Stats_" + Viewport->GetName()).c_str(), nullptr, window_flags)) {
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Memory Usage"); // 언리얼 특유의 초록색 글씨
		ImGui::Separator();
		ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

		if (ImGui::BeginTable("MemoryCounters", 4, flags)) {
			ImGui::TableSetupColumn("Memory Counters", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Current Use", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Mem%", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("Pool", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableHeadersRow();

			const char* CategoryNames[] = { "Total Allocation count" };

			for (int i = 0; i < static_cast<uint32>(EMemoryCategory::All); ++i)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", MemoryStats[i].Name.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::TextColored(ImVec4(0, 1, 0, 1), MemoryStats[i].Format.c_str(), MemoryStats[i].MemoryBytes);
				ImGui::TableSetColumnIndex(3);
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", MemoryStats[i].PoolType.c_str());
			}

			ImGui::EndTable();
		}

		ImGui::End();
	}
}
