#pragma once

#include "SSplitter.h"

class SSplitterH : public SSplitter
{
public:
	SSplitterH() = default;
	virtual ~SSplitterH() = default;

public:
	virtual void Tick(float DeltaTime) override;

	virtual FReply OnMouseButtonDown(float X, float Y) override;
	virtual FReply OnMouseButtonUp(float X, float Y) override;
	virtual FReply OnMouseMove(float X, float Y) override;
};
