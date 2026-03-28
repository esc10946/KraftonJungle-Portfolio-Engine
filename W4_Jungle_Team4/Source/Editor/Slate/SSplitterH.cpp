#include "SSplitterH.h"

void SSplitterH::Tick(float DeltaTime)
{
	FRect widgetRect = GetRect();
	uint32 SplitY = static_cast<uint32>(widgetRect.Height * SplitRatio);

	FRect topRect = { widgetRect.X, widgetRect.Y, widgetRect.Width, SplitY };
	FRect bottomRect = { widgetRect.X, widgetRect.Y + SplitY, widgetRect.Width, widgetRect.Height - SplitY };

	if (GetSideLT()) GetSideLT()->SetRect(topRect);
	if (GetSideRB()) GetSideRB()->SetRect(bottomRect);

	SSplitter::Tick(DeltaTime);

	if (GetSideLT()) GetSideLT()->Tick(DeltaTime);
	if (GetSideRB()) GetSideRB()->Tick(DeltaTime);
}

bool SSplitterH::IsOnHandle(float X, float Y) const
{
	FRect widgetRect = GetRect();
	float SplitY = widgetRect.Y + (widgetRect.Height * SplitRatio);
	return (Y >= SplitY - HandleSize * 0.5f && Y <= SplitY + HandleSize * 0.5f);
}

void SSplitterH::UpdateSplit(float X, float Y)
{
	FRect widgetRect = GetRect();
	if (widgetRect.Height > 0)
	{
		SplitRatio = (Y - widgetRect.Y) / (float)widgetRect.Height;
		if (SplitRatio < 0.05f) SplitRatio = 0.05f;
		if (SplitRatio > 0.95f) SplitRatio = 0.95f;
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