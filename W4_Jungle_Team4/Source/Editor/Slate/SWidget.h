#pragma once

#include "Math/Vector.h"

struct FReply
{
public:
	static FReply Handled() { return FReply(true); }
	static FReply Unhandled() { return FReply(false); }

	FReply& CaptureMouse(SWidget* InWidget)
	{
		MouseCaptor = InWidget;
		return *this;
	}

	bool IsHandled() const { return bIsHandled; }
	SWidget* GetMouseCaptor() const { return MouseCaptor; }

private:
	FReply(bool bInHandled) : bIsHandled(bInHandled), MouseCaptor(nullptr) {}

	bool bIsHandled;
	SWidget* MouseCaptor;
};

struct FRect
{
	FVector2 Position;
	FVector2 Size;
};

struct FPoint
{
	FVector2 Position;
};

class SWidget
{
public:
	SWidget() = default;
	virtual ~SWidget() = default;

public:
	virtual void Tick(float DeltaTime) {}

	virtual FReply OnMouseButtonDown(float X, float Y) { return FReply::Unhandled(); }
	virtual FReply OnMouseButtonUp(float X, float Y) { return FReply::Unhandled(); }
	virtual FReply OnMouseMove(float X, float Y) { return FReply::Unhandled(); }

	bool IsHovered(FPoint Coord) const;

protected:
	FRect Rect;
	bool bIsVisible = true;

private:
	SWidget* Parent = nullptr;
};