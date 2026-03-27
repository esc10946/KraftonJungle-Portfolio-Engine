#pragma once

#include "SWidget.h"

class FViewport;

class SViewport : public SWidget
{
public:
	SViewport() = default;
	virtual ~SViewport() = default;

public:
	virtual void Tick(float DeltaTime) override;

	virtual FReply OnMouseButtonDown(float X, float Y) override;
	virtual FReply OnMouseButtonUp(float X, float Y) override;
	virtual FReply OnMouseMove(float X, float Y) override;

	FViewport* GetViewport() const { return Viewport; }
	void SetViewport(FViewport* InViewport) { Viewport = InViewport; }

	void ResizeViewport();

private:
	FViewport* Viewport = nullptr;
};
