#include "SSplitterV.h"
#include "ThirdParty/ImGui/imgui.h"

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

bool SSplitterV::IsOnHandle(float X, float Y)
{
	FRect widgetRect = GetRect();
	uint32 SplitX = static_cast<uint32>(widgetRect.Width * SplitRatio);
	float CenterX = static_cast<float>(widgetRect.X + SplitX);

	bool isOn = X >= CenterX - HandleSize * 0.5f && X <= CenterX + HandleSize * 0.5f;
	if (isOn)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		Color = FColor(30, 30, 30, 255);
	}
	else
	{
		Color = FColor(30, 30, 30, 150);
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

FRect SSplitterV::GetRenderRect()
{
	FRect RenderRect;
	RenderRect.X = Rect.X + static_cast<uint32>(Rect.Width * SplitRatio) - static_cast<uint32>(HandleSize * 0.5f);
	RenderRect.Y = Rect.Y;
	RenderRect.Width = static_cast<uint32>(HandleSize);
	RenderRect.Height = Rect.Height;

	return RenderRect;
}
