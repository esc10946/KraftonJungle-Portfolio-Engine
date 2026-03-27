#pragma once

#include "SWidget.h"

class SSplitter : public SWidget
{
public:
	SSplitter() = default;
	virtual ~SSplitter() = default;

public:
	virtual void Tick(float DeltaTime) override {}

	virtual FReply OnMouseButtonDown(float X, float Y) override { return FReply::Unhandled(); }
	virtual FReply OnMouseButtonUp(float X, float Y) override { return FReply::Unhandled(); }
	virtual FReply OnMouseMove(float X, float Y) override { return FReply::Unhandled(); }

	SWidget* GetSideLT() const { return SideLT; }
	void SetSideLT(SWidget* InWidget) { SideLT = InWidget; }

	SWidget* GetSideRB() const { return SideRB; }
	void SetSideRB(SWidget* InWidget) { SideRB = InWidget; }

private:
	SWidget* SideLT = nullptr;
	SWidget* SideRB = nullptr;
};
