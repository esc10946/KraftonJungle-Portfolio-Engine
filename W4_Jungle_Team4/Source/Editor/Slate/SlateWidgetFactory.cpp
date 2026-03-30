#include "SlateWidgetFactory.h"
#include "SViewport.h"
#include "SViewportRoot.h"
#include "SSplitterH.h"
#include "SSplitterV.h"
#include "SWindow.h"
#include "SImGuiConsoleWidget.h"
#include "SImGuiControlWidget.h"
#include "SImGuiSceneWidget.h"
#include "SImGuiPropertyWidget.h"
#include "Editor/EditorEngine.h"
#include "Object/Object.h"
#include "Viewport/ViewportManager.h"
#include "Viewport/EditorViewportClient.h"

#include <fstream>
#include <filesystem>

UEditorEngine* FSlateWidgetFactory::EditorEngine = nullptr;

TMap<FString, json::JSON> FSlateWidgetFactory::LoadedWidgetData;

void FSlateWidgetFactory::LoadViewportLayouts(const FString& FilePath)
{
	std::ifstream File(std::filesystem::path(FPaths::ToWide(FilePath)));
	if (!File.is_open())
	{
		return;
	}

	FString Content((std::istreambuf_iterator<char>(File)),
		std::istreambuf_iterator<char>());

	json::JSON Root = json::JSON::Load(Content);

	for (auto& data : Root.ObjectRange())
	{
		LoadedWidgetData[data.first] = data.second;
	}
}

SWidget* FSlateWidgetFactory::CreateWidget(const FString& WidgetType)
{
	if (EditorEngine == nullptr)
	{
		for (UObject* Obj : GUObjectArray)
		{
			if (UEditorEngine* Editor = dynamic_cast<UEditorEngine*>(Obj))
			{
				EditorEngine = Editor;
				break;
			}
		}
	}

	if (WidgetType == "SWindow")
	{
		return new SWindow();
	}
	else if (WidgetType == "SViewport")
	{
		return new SViewport();
	}
	else if (WidgetType == "SViewportRoot")
	{
		SViewportRoot* ViewportRootWidget = new SViewportRoot();
		return ViewportRootWidget;
	}
	else if (WidgetType == "SSplitterH")
	{
		return new SSplitterH();
	}
	else if (WidgetType == "SSplitterV")
	{
		return new SSplitterV();
	}
	else if (WidgetType == "SImGuiConsoleWidget")
	{
		return new SImGuiConsoleWidget();
	}
	else if (WidgetType == "SImGuiControlWidget")
	{
		return new SImGuiControlWidget(EditorEngine);
	}
	else if (WidgetType == "SImGuiSceneWidget")
	{
		return new SImGuiSceneWidget(EditorEngine);
	}
	else if (WidgetType == "SImGuiPropertyWidget")
	{
		return new SImGuiPropertyWidget(EditorEngine);
	}

	return nullptr;
}

SWidget* FSlateWidgetFactory::LoadRecursive(json::JSON& Data)
{
	FString WidgetType = Data["WidgetType"].ToString();
	SWidget* Widget = CreateWidget(WidgetType);
	if (Widget)
	{
		Widget->Deserialize(Data);

		if (SSplitter* Splitter = dynamic_cast<SSplitter*>(Widget))
		{
			if (Data.hasKey("SideLT"))
			{
				SWidget* SideLT = LoadRecursive(Data["SideLT"]);
				if (SideLT)
				{
					Splitter->SetSideLT(SideLT);
				}
			}
			if (Data.hasKey("SideRB"))
			{
				SWidget* SideRB = LoadRecursive(Data["SideRB"]);
				if (SideRB)
				{
					Splitter->SetSideRB(SideRB);
				}
			}
		}
		else
		{
			for (auto& ChildData : Data["Children"].ArrayRange())
			{
				SWidget* ChildWidget = LoadRecursive(ChildData);
				if (ChildWidget)
				{
					Widget->AddChild(ChildWidget);
				}
			}
		}
	}

	return Widget;
}

