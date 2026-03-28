#pragma once

#include "Core/EngineTypes.h"
#include "Reply.h"

struct FRect
{
	uint32 X, Y;
	uint32 Width, Height;
};

class SWidget
{
public:
	SWidget() = default;
	virtual ~SWidget() = default;

public:
	virtual void Tick(float DeltaTime);
	virtual void OnPaint();

	virtual FReply OnMouseButtonDown(float X, float Y, int32 Button) { return FReply::Unhandled(); }
	virtual FReply OnMouseButtonUp(float X, float Y, int32 Button) { return FReply::Unhandled(); }
	virtual FReply OnMouseMove(float X, float Y) { return FReply::Unhandled(); }
	virtual FReply OnMouseWheel(float Delta) { return FReply::Unhandled(); }

	virtual void OnKeyDown(int32 Key) {}
	virtual void OnKeyUp(int32 Key) {}

	bool IsHovered(FVector2 Coord) const;

	FRect GetRect() const { return Rect; }
	void SetRect(const FRect& InRect) { Rect = InRect; }

	void AddChild(SWidget* InChild)
	{
		Children.push_back(InChild);
		InChild->Parent = this;
	}
	virtual TArray<SWidget*> GetChildren() { return Children; }

protected:
	FRect Rect;
	bool bIsVisible = true;

	SWidget* Parent = nullptr;
	TArray<SWidget*> Children;
};