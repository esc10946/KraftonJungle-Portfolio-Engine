#include "SSplitterH.h"

void SSplitterH::Tick(float DeltaTime)
{
	FRect widgetRect = GetRect();
	FRect topRect = { widgetRect.X, widgetRect.Y, widgetRect.Width, widgetRect.Height / 2 };
	FRect bottomRect = { widgetRect.X, widgetRect.Y + widgetRect.Height / 2, widgetRect.Width, widgetRect.Height / 2 };

	GetSideLT()->SetRect(topRect);
	GetSideRB()->SetRect(bottomRect);

	SSplitter::Tick(DeltaTime);

	GetSideLT()->Tick(DeltaTime);
	GetSideRB()->Tick(DeltaTime);
}

FReply SSplitterH::OnMouseButtonDown(float X, float Y)
{
	return SSplitter::OnMouseButtonDown(X, Y);
}

FReply SSplitterH::OnMouseButtonUp(float X, float Y)
{
	return SSplitter::OnMouseButtonUp(X, Y);
}

FReply SSplitterH::OnMouseMove(float X, float Y)
{
	return SSplitter::OnMouseMove(X, Y);
}