#pragma once

#include "SSplitter.h"

class SSplitterV : public SSplitter
{
public:
	SSplitterV() = default;
	virtual ~SSplitterV() = default;

public:
	virtual void Tick(float DeltaTime) override;

	virtual FReply OnMouseButtonDown(float X, float Y, int32 Button) override;
	virtual FReply OnMouseButtonUp(float X, float Y, int32 Button) override;
	virtual FReply OnMouseMove(float X, float Y) override;

	virtual FString GetWidgetType() const { return "SSplitterV"; }

protected:
	virtual bool IsOnHandle(float X, float Y) const override;
	virtual void UpdateSplit(float X, float Y) override;
};
