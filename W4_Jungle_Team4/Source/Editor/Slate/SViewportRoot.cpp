#include "SViewportRoot.h"
#include "SViewport.h"
#include "SSplitterH.h"
#include "SSplitterV.h"
#include "Viewport/ViewportManager.h"
#include "Viewport/EditorViewportClient.h"

SViewportRoot::SViewportRoot()
{
	// 초기에 미리 4개의 뷰포트를 생성합니다.
	for (int i = 0; i < 4; ++i)
	{
		FViewport* Viewport = FViewportManager::Get().CreateViewport();
		FEditorViewportClient* Client = FViewportManager::Get().CreateEditorViewportClient();
		Client->Initialize(Viewport);
		Viewport->SetViewportClient(Client);

		SViewport* ViewportWidget = new SViewport();
		ViewportWidget->SetViewport(Viewport);

		Viewports.push_back(ViewportWidget);

		ViewportWidget->SetName("Viewport" + std::to_string(i));
	}

	// 미리 viewport들의 option을 지정합니다.
	EViewOption DefaultOptions[] = { EViewOption::Perspective, EViewOption::Top, EViewOption::Left, EViewOption::Front };
	for (int i = 0; i < 4; ++i)
	{
		FEditorViewportClient* Client = static_cast<FEditorViewportClient*>(Viewports[i]->GetViewport()->GetViewportClient());
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

	if (VisibleCount == 1)
	{
		// Single Viewport Layout
		AddChild(Viewports[0]);
		Viewports[0]->SetRect(Rect);
	}
	else
	{
		// Multi Viewport Layout (2x2)
		SSplitterH* Splitter = new SSplitterH();
		Splitter->SetRect(Rect);

		SSplitterV* SplitterV = new SSplitterV();
		SSplitterV* SplitterV2 = new SSplitterV();

		Splitter->SetSideLT(SplitterV);
		Splitter->SetSideRB(SplitterV2);
		AddChild(Splitter);

		SplitterV->SetSideLT(Viewports[0]);
		SplitterV->SetSideRB(Viewports[1]);
		SplitterV2->SetSideLT(Viewports[2]);
		SplitterV2->SetSideRB(Viewports[3]);
	}
}
