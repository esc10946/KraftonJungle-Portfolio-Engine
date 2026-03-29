#pragma once

#include "Core/EngineTypes.h"
#include "Reply.h"
#include "ThirdParty/SimpleJSON/json.hpp"

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

	const FString& GetName() const { return Name; }
	void SetName(const FString& InName) { Name = InName; }

	void AddChild(SWidget* InChild)
	{
		Children.push_back(InChild);
		InChild->Parent = this;
	}
	virtual TArray<SWidget*> GetChildren() { return Children; }
	void RemoveChildren()
	{
		for (SWidget* Child : Children)
		{
			Child->Parent = nullptr;
		}
		Children.clear();
	}

	SWidget* GetParent() const { return Parent; }
	void SetParent(SWidget* InParent) { Parent = InParent; }

	virtual FString GetWidgetType() const { return "SWidget"; }
	virtual json::JSON Serialize() const;
	virtual void Deserialize(json::JSON& Data);

protected:
	FString Name;

	FRect Rect;
	bool bIsVisible = true;

	SWidget* Parent = nullptr;
	TArray<SWidget*> Children;
};