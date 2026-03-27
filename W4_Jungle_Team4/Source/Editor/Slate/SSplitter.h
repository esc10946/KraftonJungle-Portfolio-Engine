#pragma once

#include "SWidget.h"

class SSplitter : public SWidget
{
public:
	SSplitter() = default;
	virtual ~SSplitter() = default;

public:
	virtual void Tick(float DeltaTime) override;

	virtual FReply OnMouseButtonDown(float X, float Y) override;
	virtual FReply OnMouseButtonUp(float X, float Y) override;
	virtual FReply OnMouseMove(float X, float Y) override;

private:
	SWidget* SideLT = nullptr;
	SWidget* SideRB = nullptr;
};
