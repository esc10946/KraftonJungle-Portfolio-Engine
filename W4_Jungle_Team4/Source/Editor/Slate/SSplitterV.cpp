#include "SSplitterV.h"
#include "UI/EditorConsoleWidget.h"

void SSplitterV::Tick(float DeltaTime)
{
	SSplitter::Tick(DeltaTime);

	if (GetSideLT()) GetSideLT()->Tick(DeltaTime);
	if (GetSideRB()) GetSideRB()->Tick(DeltaTime);
}

void SSplitterV::OnResize(const FRect& NewRect)
{
	Rect = NewRect;
	uint32 SplitX = static_cast<uint32>(Rect.Width * SplitRatio);

	FRect leftRect = { Rect.X, Rect.Y, SplitX, Rect.Height };
	FRect rightRect = { Rect.X + SplitX, Rect.Y, Rect.Width - SplitX, Rect.Height };

	if (GetSideLT()) GetSideLT()->OnResize(leftRect);
	if (GetSideRB()) GetSideRB()->OnResize(rightRect);
}

bool SSplitterV::IsOnHandle(float X, float Y) const
{
	FRect widgetRect = GetRect();
	float SplitX = widgetRect.X + (widgetRect.Width * SplitRatio);

	bool isOn = X >= SplitX - HandleSize * 0.5f && X <= SplitX + HandleSize * 0.5f;
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

void SSplitterV::UpdateSplit(float X, float Y)
{
	FRect widgetRect = GetRect();
	if (widgetRect.Width > 0)
	{
		SplitRatio = (X - widgetRect.X) / (float)widgetRect.Width;
		if (SplitRatio < 0.05f) SplitRatio = 0.05f;
		if (SplitRatio > 0.95f) SplitRatio = 0.95f;

		OnResize(widgetRect);
	}
}

FReply SSplitterV::OnMouseButtonDown(float X, float Y, int32 Button)
{
	return SSplitter::OnMouseButtonDown(X, Y, Button);
}

FReply SSplitterV::OnMouseButtonUp(float X, float Y, int32 Button)
{
	return SSplitter::OnMouseButtonUp(X, Y, Button);
}

FReply SSplitterV::OnMouseMove(float X, float Y)
{
	return SSplitter::OnMouseMove(X, Y);
}
