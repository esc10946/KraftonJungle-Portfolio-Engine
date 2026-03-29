#pragma once

#include "SWidget.h"

class UEditorEngine;

class SImGuiSceneWidget : public SWidget
{
public:
	SImGuiSceneWidget(UEditorEngine* InEditorEngine);
	virtual ~SImGuiSceneWidget() = default;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPaint() override;

	virtual FString GetWidgetType() const { return "SImGuiSceneWidget"; }

private:
	void RefreshSceneFileList();

	UEditorEngine* EditorEngine;

	char SceneName[128] = "Default";

	TArray<FString> SceneFiles;
	int32 SelectedSceneIndex = -1;

	float NewSceneNotificationTimer = 0.f;
	float SceneSaveNotificationTimer = 0.f;
	float SceneLoadNotificationTimer = 0.f;
};