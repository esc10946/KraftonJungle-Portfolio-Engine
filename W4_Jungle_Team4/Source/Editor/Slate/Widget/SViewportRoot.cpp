#include "SViewportRoot.h"
#include "SViewport.h"
#include "SSplitterH.h"
#include "SSplitterV.h"
#include "Slate/SlateWidgetFactory.h"
#include "Slate/SlateApplication.h"
#include "Viewport/ViewportManager.h"
#include "Viewport/EditorViewportClient.h"
#include "Core/TweenSystem.h"

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
	FSlateApplication::Get().ClearInputCaptor();
	RemoveChildren();

	FString LayoutName = LayoutToName(CurrentLayout);
	SWidget* Root = FSlateWidgetFactory::GetViewportLayout(LayoutName);

	if (Root)
	{
		AddChild(Root);

		TArray<SViewport*> ViewportWidgets = FSlateApplication::Get().GetWidgetsByType<SViewport*>(Root);
		TArray<SSplitter*> Splitters = FSlateApplication::Get().GetWidgetsByType<SSplitter*>(Root);

		for (size_t i = 0; i < ViewportWidgets.size(); ++i)
		{
			if (i < FViewportManager::Get().GetViewports().size())
			{
				ViewportWidgets[i]->SetViewport(FViewportManager::Get().GetViewports()[i]);
				ViewportWidgets[i]->SetName("Viewport " + std::to_string(i + 1));
			}
		}

		for (size_t i = 0; i < Splitters.size(); ++i)
		{
			if (i < LayoutInfos[static_cast<int32>(CurrentLayout)].SplitRatios.size())
			{
				float TargetSplitRatio = LayoutInfos[static_cast<int32>(CurrentLayout)].SplitRatios[i];

				FTweenSystem::Get().Create(0.05f, TargetSplitRatio, 0.2f)
					.SetEasing(EEasingType::EaseInOut)
					.SetUpdateCallback([Splitter = Splitters[i]](float Value) {
					Splitter->SetSplitRatio(Value);
					Splitter->OnResize(Splitter->GetRect());
					});
			}
		}
	}
}

void SViewportRoot::SaveCurrentLayoutInfo()
{
	FViewportLayoutInfo& Info = LayoutInfos[static_cast<int32>(CurrentLayout)];
	Info.Layout = CurrentLayout;
	TArray<SSplitter*> Splitters = FSlateApplication::Get().GetWidgetsByType<SSplitter*>(this);
	Info.SplitRatios.clear();
	for (SSplitter* Splitter : Splitters)
	{
		Info.SplitRatios.push_back(Splitter->GetSplitRatio());
	}
}

FString SViewportRoot::LayoutToName(EViewportLayout Layout)
{
	static const TMap<EViewportLayout, FString> LayoutMap = {
		{ EViewportLayout::Single, "Single" },
		{ EViewportLayout::TwoHorizontal, "TwoHorizontal" },
		{ EViewportLayout::TwoVertical, "TwoVertical" },
		{ EViewportLayout::ThreeTopMain, "ThreeTopMain" },
		{ EViewportLayout::ThreeBottomMain, "ThreeBottomMain" },
		{ EViewportLayout::ThreeLeftMain, "ThreeLeftMain" },
		{ EViewportLayout::ThreeRightMain, "ThreeRightMain" },
		{ EViewportLayout::FourQuad, "FourQuad" },
		{ EViewportLayout::FourTopMain, "FourTopMain" },
		{ EViewportLayout::FourBottomMain, "FourBottomMain" },
		{ EViewportLayout::FourLeftMain, "FourLeftMain" },
		{ EViewportLayout::FourRightMain, "FourRightMain" }
	};

	auto it = LayoutMap.find(Layout);
	if (it != LayoutMap.end())
	{
		return it->second;
	}

	return "Single";
}

json::JSON SViewportRoot::Serialize()
{
	SaveCurrentLayoutInfo();

	json::JSON Data = SWidget::Serialize();
	Data["Layout"] = static_cast<int32>(CurrentLayout);

	Data["LayoutInfos"] = json::Array();
	for (const FViewportLayoutInfo& Info : LayoutInfos)
	{
		json::JSON InfoData = json::Object();
		InfoData["Layout"] = static_cast<int32>(Info.Layout);
		InfoData["SplitRatios"] = json::Array();
		for (float Ratio : Info.SplitRatios)
		{
			InfoData["SplitRatios"].append(Ratio);
		}

		Data["LayoutInfos"].append(InfoData);
	}

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

	if (Data.hasKey("LayoutInfos"))
	{
		for (json::JSON& InfoData : Data["LayoutInfos"].ArrayRange())
		{
			FViewportLayoutInfo Info;
			Info.Layout = static_cast<EViewportLayout>(InfoData["Layout"].ToInt());
			for (const json::JSON& Ratio : InfoData["SplitRatios"].ArrayRange())
			{
				Info.SplitRatios.push_back(static_cast<float>(Ratio.ToFloat()));
			}
			int32 LayoutIndex = static_cast<int32>(Info.Layout);
			if (LayoutIndex >= 0 && LayoutIndex < static_cast<int32>(EViewportLayout::All))
			{
				LayoutInfos[LayoutIndex] = Info;
			}
		}
	}
}
