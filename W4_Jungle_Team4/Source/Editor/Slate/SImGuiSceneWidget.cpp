#include "SImGuiSceneWidget.h"

#include "Editor/EditorEngine.h"
#include "Engine/Core/Common.h"
#include "GameFramework/WorldContext.h"

#include "ImGui/imgui.h"
#include "Component/GizmoComponent.h"
#include "Serialization/SceneSaveManager.h"

#include <filesystem>

#define SEPARATOR(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

SImGuiSceneWidget::SImGuiSceneWidget(UEditorEngine* InEditorEngine)
	: EditorEngine(InEditorEngine)
{
	RefreshSceneFileList();
}

void SImGuiSceneWidget::RefreshSceneFileList()
{
	SceneFiles = FSceneSaveManager::GetSceneFileList();
	if (SelectedSceneIndex >= static_cast<int32>(SceneFiles.size()))
	{
		SelectedSceneIndex = SceneFiles.empty() ? -1 : 0;
	}
}

void SImGuiSceneWidget::Tick(float DeltaTime)
{
	SWidget::Tick(DeltaTime);
	if (NewSceneNotificationTimer > 0.f) NewSceneNotificationTimer -= DeltaTime;
	if (SceneSaveNotificationTimer > 0.f) SceneSaveNotificationTimer -= DeltaTime;
	if (SceneLoadNotificationTimer > 0.f) SceneLoadNotificationTimer -= DeltaTime;
}

void SImGuiSceneWidget::OnPaint()
{
	using namespace common::constants::ImGui;

	if (!EditorEngine)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2((float)Rect.X, (float)Rect.Y));
	ImGui::SetNextWindowSize(ImVec2((float)Rect.Width, (float)Rect.Height));

	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

	ImGui::Begin("Jungle Scene Manager", nullptr, WindowFlags);

	// New Scene
	if (ImGui::Button("New Scene"))
	{
		EditorEngine->NewScene();
		NewSceneNotificationTimer = NotificationTimer;
	}
	if (NewSceneNotificationTimer > 0.0f)
	{
		ImGui::SameLine();
		ImGui::Text("New scene created");
	}

	SEPARATOR();

	// Save Scene
	ImGui::InputText("Scene Name", SceneName, IM_ARRAYSIZE(SceneName));

	if (ImGui::Button("Save Scene"))
	{
		FWorldContext* Ctx = EditorEngine->GetWorldContextFromHandle(EditorEngine->GetActiveWorldHandle());
		if (Ctx) FSceneSaveManager::SaveSceneAsJSON(SceneName, *Ctx);
		SceneSaveNotificationTimer = NotificationTimer;
		RefreshSceneFileList();
	}
	if (SceneSaveNotificationTimer > 0.0f)
	{
		ImGui::SameLine();
		ImGui::Text("Scene saved");
	}

	SEPARATOR();

	// Load Scene (combo box)
	if (ImGui::Button("Refresh"))
	{
		RefreshSceneFileList();
	}
	ImGui::SameLine();
	ImGui::Text("Scene Files (%d)", static_cast<int32>(SceneFiles.size()));

	if (!SceneFiles.empty())
	{
		auto SceneNameGetter = [](void* Data, int32 Idx) -> const char*
			{
				auto* Files = static_cast<TArray<FString>*>(Data);
				if (Idx < 0 || Idx >= static_cast<int32>(Files->size())) return nullptr;
				return (*Files)[Idx].c_str();
			};

		ImGui::Combo("Scene File", &SelectedSceneIndex, SceneNameGetter, &SceneFiles, static_cast<int32>(SceneFiles.size()));

		if (ImGui::Button("Load Scene") && SelectedSceneIndex >= 0)
		{
			std::filesystem::path ScenePath = std::filesystem::path(FSceneSaveManager::GetSceneDirectory())
				/ (FPaths::ToWide(SceneFiles[SelectedSceneIndex]) + FSceneSaveManager::SceneExtension);
			FString FilePath = FPaths::ToUtf8(ScenePath.wstring());

			EditorEngine->ClearScene();
			FWorldContext LoadCtx;
			FSceneSaveManager::LoadSceneFromJSON(FilePath, LoadCtx);
			if (LoadCtx.World)
			{
				EditorEngine->GetWorldList().push_back(LoadCtx);
				EditorEngine->SetActiveWorld(LoadCtx.ContextHandle);
			}
			EditorEngine->ResetViewport();
			SceneLoadNotificationTimer = NotificationTimer;
		}
		if (SceneLoadNotificationTimer > 0.0f)
		{
			ImGui::SameLine();
			ImGui::Text("Scene loaded");
		}
	}
	else
	{
		ImGui::Text("No scene files found in %s", FPaths::ToUtf8(FSceneSaveManager::GetSceneDirectory()).c_str());
	}

	SEPARATOR();

	// Actor Outliner
	UWorld* World = EditorEngine->GetWorld();
	if (World)
	{
		const TArray<AActor*>& Actors = World->GetActors();
		ImGui::Text("Actors (%d)", static_cast<int32>(Actors.size()));
		ImGui::Separator();

		// Fill remaining space with scrollable child region
		FSelectionManager& Selection = EditorEngine->GetSelectionManager();

		ImGui::BeginChild("ActorList", ImVec2(0, 0), ImGuiChildFlags_Borders);
		for (AActor* Actor : Actors)
		{
			if (!Actor) continue;

			FString ActorName = Actor->GetFName().ToString();
			if (ActorName.empty())
			{
				ActorName = Actor->GetTypeInfo()->name;
			}

			bool bIsSelected = Selection.IsSelected(Actor);
			if (ImGui::Selectable(ActorName.c_str(), bIsSelected))
			{
				if (ImGui::GetIO().KeyShift)
				{
					Selection.SelectRange(Actor, Actors);
				}
				else if (ImGui::GetIO().KeyCtrl)
				{
					Selection.ToggleSelect(Actor);
				}
				else
				{
					Selection.Select(Actor);
				}
			}
		}
		ImGui::EndChild();
	}

	ImGui::End();
}