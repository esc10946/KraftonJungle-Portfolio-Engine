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
	virtual void Tick(float DeltaTime) { if (Child) Child->Tick(DeltaTime); }

	virtual FReply OnMouseButtonDown(float X, float Y) { return FReply::Unhandled(); }
	virtual FReply OnMouseButtonUp(float X, float Y) { return FReply::Unhandled(); }
	virtual FReply OnMouseMove(float X, float Y) { return FReply::Unhandled(); }

	bool IsHovered(FVector2 Coord) const;

	FRect GetRect() const { return Rect; }
	void SetRect(const FRect& InRect) { Rect = InRect; }

	void AddChild(SWidget* InChild)
	{
		if (Child)
		{
			Child->Parent = nullptr;
		}
		Child = InChild;
		if (Child)
		{
			Child->Parent = this;
		}
	}

protected:
	FRect Rect;
	bool bIsVisible = true;

	SWidget* Parent = nullptr;
	SWidget* Child = nullptr;
};