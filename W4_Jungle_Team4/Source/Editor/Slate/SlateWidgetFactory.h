#include "SWidget.h"
#include "Core/EngineTypes.h"
#include "Core/Paths.h"

class UEditorEngine;

class FSlateWidgetFactory
{
public:
	static void LoadViewportLayouts(const FString& FilePath = FPaths::ToUtf8(FPaths::EditorLayoutFilePath()));
	static SWidget* GetViewportLayout(const FString& LayoutName);

	static SWidget* CreateWidget(const FString& WidgetType);

	static SWidget* LoadRecursive(json::JSON& Data);
	static SWidget* LoadRecursive(const FString& WidgetKey)
	{
		if (LoadedWidgetData.find(WidgetKey) != LoadedWidgetData.end())
		{
			return LoadRecursive(LoadedWidgetData[WidgetKey]);
		}
		return nullptr;
	}

private:
	static UEditorEngine* EditorEngine; // Assume this is set externally before loading widgets

	static TMap<FString, json::JSON> LoadedWidgetData; // Cache for loaded widget data, keyed by some identifier (e.g., widget name or type)
};