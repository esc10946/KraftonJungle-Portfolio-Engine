#pragma once

#include "SWidget.h"
#include <windows.h>

class SSplitter : public SWidget
{
public:
	SSplitter() { bRenderable = true; }
	virtual ~SSplitter() = default;

public:
	virtual void Tick(float DeltaTime) override {}

	virtual FReply OnMouseButtonDown(float X, float Y, int32 Button) override
	{
		if (Button == VK_LBUTTON && IsOnHandle(X, Y))
		{
			bIsDragging = true;
			return FReply::Handled().CaptureMouse(this);
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(float X, float Y, int32 Button) override
	{
		if (Button == VK_LBUTTON && bIsDragging)
		{
			bIsDragging = false;
			return FReply::Handled().CaptureMouse(nullptr);
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseMove(float X, float Y) override
	{
		if (bIsDragging)
		{
			UpdateSplit(X, Y);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	SWidget* GetSideLT() const { return SideLT; }
	void SetSideLT(SWidget* InWidget) { SideLT = InWidget; InWidget->SetParent(this); }

	SWidget* GetSideRB() const { return SideRB; }
	void SetSideRB(SWidget* InWidget) { SideRB = InWidget; InWidget->SetParent(this); }

	float GetSplitRatio() const { return SplitRatio; }
	void SetSplitRatio(float InRatio) { SplitRatio = InRatio; }

	virtual TArray<SWidget*> GetChildren() override
	{
		TArray<SWidget*> Children;
		if (SideLT) Children.push_back(SideLT);
		if (SideRB) Children.push_back(SideRB);
		return Children;
	}

	virtual bool IsOnHandle(float X, float Y) { return false; }
	virtual void UpdateSplit(float X, float Y) {}

	virtual json::JSON Serialize() override
	{
		json::JSON Data = SWidget::Serialize();
		Data["SplitRatio"] = SplitRatio;

		if (SideLT) Data["SideLT"] = SideLT->Serialize();
		if (SideRB) Data["SideRB"] = SideRB->Serialize();

		return Data;
	}

	virtual void Deserialize(json::JSON& Data) override
	{
		SWidget::Deserialize(Data);
		if (Data.hasKey("SplitRatio"))
		{
			SplitRatio = Data["SplitRatio"].ToFloat();
		}
	}

protected:
	SWidget* SideLT = nullptr;
	SWidget* SideRB = nullptr;

	float SplitRatio = 0.5f;
	bool bIsDragging = false;
	const float HandleSize = 10.0f;
};
