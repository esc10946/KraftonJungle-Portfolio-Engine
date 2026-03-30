#include "SSplitterH.h"
#include "Editor/UI/EditorConsoleWidget.h"

void SSplitterH::Tick(float DeltaTime)
{
	SSplitter::Tick(DeltaTime);

	if (GetSideLT()) GetSideLT()->Tick(DeltaTime);
	if (GetSideRB()) GetSideRB()->Tick(DeltaTime);
}

void SSplitterH::OnResize(const FRect& NewRect)
{
	Rect = NewRect;
	uint32 SplitY = static_cast<uint32>(Rect.Height * SplitRatio);

	FRect topRect = { Rect.X, Rect.Y, Rect.Width, SplitY };
	FRect bottomRect = { Rect.X, Rect.Y + SplitY, Rect.Width, Rect.Height - SplitY };

	if (GetSideLT()) GetSideLT()->OnResize(topRect);
	if (GetSideRB()) GetSideRB()->OnResize(bottomRect);
}

bool SSplitterH::IsOnHandle(float X, float Y) const
{
	FRect widgetRect = GetRect();
	float SplitY = widgetRect.Y + (widgetRect.Height * SplitRatio);

	bool isOn = Y >= SplitY - HandleSize * 0.5f && Y <= SplitY + HandleSize * 0.5f;
	if (isOn)
	{
		SetCursor(LoadCursor(NULL, IDC_CROSS));
	}
	else
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}

	return isOn;
}

void SSplitterH::UpdateSplit(float X, float Y)
{
	FRect widgetRect = GetRect();
	if (widgetRect.Height > 0)
	{
		SplitRatio = (Y - widgetRect.Y) / (float)widgetRect.Height;
		if (SplitRatio < 0.05f) SplitRatio = 0.05f;
		if (SplitRatio > 0.95f) SplitRatio = 0.95f;

		OnResize(widgetRect);
	}
}

FReply SSplitterH::OnMouseButtonDown(float X, float Y, int32 Button)
{
	return SSplitter::OnMouseButtonDown(X, Y, Button);
}

FReply SSplitterH::OnMouseButtonUp(float X, float Y, int32 Button)
{
	return SSplitter::OnMouseButtonUp(X, Y, Button);
}

FReply SSplitterH::OnMouseMove(float X, float Y)
{
	return SSplitter::OnMouseMove(X, Y);
}