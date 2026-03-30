#include "SViewportRoot.h"
#include "SViewport.h"
#include "SSplitterH.h"
#include "SSplitterV.h"
#include "SlateWidgetFactory.h"
#include "SlateApplication.h"
#include "Viewport/ViewportManager.h"
#include "Viewport/EditorViewportClient.h"

SViewportRoot::SViewportRoot()
{
	// 초기에 미리 4개의 뷰포트를 생성합니다.
	EViewOption DefaultOptions[] = { EViewOption::Perspective, EViewOption::Top, EViewOption::Left, EViewOption::Front };

	for (int i = 0; i < 4; ++i)
	{
		FViewport* Viewport = FViewportManager::Get().CreateViewport();
		FEditorViewportClient* Client = FViewportManager::Get().CreateEditorViewportClient();
		Client->Initialize(Viewport);
		Viewport->SetViewportClient(Client);

		Client->SetCurrentViewOption(DefaultOptions[i]);
	}
}

SViewportRoot::~SViewportRoot()
{
	for (SViewport* ViewportWidget : Viewports)
	{
		if (ViewportWidget)
		{
			FViewport* Viewport = ViewportWidget->GetViewport();
			if (Viewport)
			{
				FViewportManager::Get().DestroyViewport(Viewport);
			}
		}
	}
}

void SViewportRoot::Tick(float DeltaTime)
{
	SWidget::Tick(DeltaTime);

	if (GetChildren().size() == 1)
	{
		GetChildren()[0]->SetRect(Rect);
	}

	if (bDirty)
	{
		Construct();
		OnResize(Rect);
		bDirty = false;
	}
}

void SViewportRoot::Construct()
{
	RemoveChildren();

	FString LayoutName = LayoutToName(CurrentLayout);
	SWidget* Root = FSlateWidgetFactory::GetViewportLayout(LayoutName);

	if (Root)
	{
		AddChild(Root);

		TArray<SViewport*> ViewportWidgets = FSlateApplication::Get().GetWidgetsByType<SViewport*>(Root);

		for (size_t i = 0; i < ViewportWidgets.size(); ++i)
		{
			if (i < FViewportManager::Get().GetViewports().size())
			{
				ViewportWidgets[i]->SetViewport(FViewportManager::Get().GetViewports()[i]);
				ViewportWidgets[i]->SetName("Viewport " + std::to_string(i + 1));
			}
		}
	}
}

FString SViewportRoot::LayoutToName(EViewportLayout Layout)
{
	static const TMap<EViewportLayout, FString> LayoutMap = {
		{ EViewportLayout::Single, "Single" },
		{ EViewportLayout::TwoHorizontal, "Two-Horizontal" },
		{ EViewportLayout::TwoVertical, "Two-Vertical" },
		{ EViewportLayout::Four, "Four" }
	};

	auto it = LayoutMap.find(Layout);
	if (it != LayoutMap.end())
	{
		return it->second;
	}

	return "Single";
}

json::JSON SViewportRoot::Serialize() const
{
	json::JSON Data = SWidget::Serialize();
	Data["Layout"] = static_cast<int32>(CurrentLayout);
	return Data;
}

void SViewportRoot::Deserialize(json::JSON& Data)
{
	SWidget::Deserialize(Data);
	if (Data.hasKey("Layout"))
	{
		CurrentLayout = static_cast<EViewportLayout>(Data["Layout"].ToInt());
		bDirty = true;
	}
}
