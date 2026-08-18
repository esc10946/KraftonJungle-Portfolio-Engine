#pragma once

#include "SWidget.h"

class UEditorEngine;
class UActorComponent;
class AActor;

class SImGuiPropertyWidget : public SWidget
{
public:
	SImGuiPropertyWidget(UEditorEngine* InEditorEngine);
	virtual ~SImGuiPropertyWidget() = default;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPaint() override;

	virtual FString GetWidgetType() const { return "SImGuiPropertyWidget"; }

private:
	void RenderComponentTree(AActor* Actor);
	void RenderSceneComponentNode(class USceneComponent* Comp);
	void RenderDetails(AActor* PrimaryActor, const TArray<AActor*>& SelectedActors);
	void RenderComponentProperties();
	void RenderActorProperties(AActor* PrimaryActor, const TArray<AActor*>& SelectedActors);
	void RenderPropertyWidget(struct FPropertyDescriptor& Prop);

	UEditorEngine* EditorEngine;

	UActorComponent* SelectedComponent = nullptr;
	AActor* LastSelectedActor = nullptr;
	bool bActorSelected = true; // true: Actor details, false: Component details
	bool bAbortPropertyRender = false;
};
