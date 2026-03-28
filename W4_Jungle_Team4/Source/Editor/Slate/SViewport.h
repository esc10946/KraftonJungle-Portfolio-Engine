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

	virtual FReply OnMouseButtonDown(float X, float Y, int32 Button) override;
	virtual FReply OnMouseButtonUp(float X, float Y, int32 Button) override;
	virtual FReply OnMouseMove(float X, float Y) override;
	virtual FReply OnMouseWheel(float Delta) override;

	virtual void OnKeyDown(int32 Key) override;
	virtual void OnKeyUp(int32 Key) override;

	FViewport* GetViewport() const { return Viewport; }
	void SetViewport(FViewport* InViewport) { Viewport = InViewport; }

	virtual FString GetWidgetType() const { return "SViewport"; }

private:
	FViewport* Viewport = nullptr;
};
