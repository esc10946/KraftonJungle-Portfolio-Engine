#include "SWidget.h"

class UEditorEngine;

class FSlateWidgetFactory
{
public:
	static SWidget* CreateWidget(const FString& WidgetType);

	static SWidget* LoadRecursive(json::JSON& Data);

private:
	static UEditorEngine* EditorEngine; // Assume this is set externally before loading widgets
};