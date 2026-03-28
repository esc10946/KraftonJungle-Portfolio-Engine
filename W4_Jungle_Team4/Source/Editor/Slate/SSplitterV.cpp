#include "SSplitterV.h"

void SSplitterV::Tick(float DeltaTime)
{
	FRect widgetRect = GetRect();
	uint32 SplitX = static_cast<uint32>(widgetRect.Width * SplitRatio);

	FRect leftRect = { widgetRect.X, widgetRect.Y, SplitX, widgetRect.Height };
	FRect rightRect = { widgetRect.X + SplitX, widgetRect.Y, widgetRect.Width - SplitX, widgetRect.Height };

	if (GetSideLT()) GetSideLT()->SetRect(leftRect);
	if (GetSideRB()) GetSideRB()->SetRect(rightRect);

	SSplitter::Tick(DeltaTime);

	if (GetSideLT()) GetSideLT()->Tick(DeltaTime);
	if (GetSideRB()) GetSideRB()->Tick(DeltaTime);
}

bool SSplitterV::IsOnHandle(float X, float Y) const
{
	FRect widgetRect = GetRect();
	float SplitX = widgetRect.X + (widgetRect.Width * SplitRatio);
	return (X >= SplitX - HandleSize * 0.5f && X <= SplitX + HandleSize * 0.5f);
}

void SSplitterV::UpdateSplit(float X, float Y)
{
	FRect widgetRect = GetRect();
	if (widgetRect.Width > 0)
	{
		SplitRatio = (X - widgetRect.X) / (float)widgetRect.Width;
		if (SplitRatio < 0.05f) SplitRatio = 0.05f;
		if (SplitRatio > 0.95f) SplitRatio = 0.95f;
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
