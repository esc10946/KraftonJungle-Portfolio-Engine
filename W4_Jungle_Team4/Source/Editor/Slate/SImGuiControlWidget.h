#pragma once

#include "SWidget.h"

class UEditorEngine;

class SImGuiControlWidget : public SWidget
{
public:
	SImGuiControlWidget(UEditorEngine* InEditorEngine);
	virtual ~SImGuiControlWidget() = default;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPaint() override;

	virtual FString GetWidgetType() const { return "SImGuiControlWidget"; }

private:
	UEditorEngine* EditorEngine;

	const char* PrimitiveTypes[4] = { "Cube", "Sphere", "Plane", "Explosion" };
	int32 SelectedPrimitiveType = 0;
	int32 NumberOfSpawnedActors = 1;
	FVector CurSpawnPoint = { 0.f, 0.f, 0.f };
};