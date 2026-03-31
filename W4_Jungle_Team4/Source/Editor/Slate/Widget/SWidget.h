#pragma once

#include "Core/EngineTypes.h"
#include "Slate/Reply.h"
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
	virtual void OnResize(const FRect& NewRect);

	virtual FReply OnMouseButtonDown(float X, float Y, int32 Button) { return FReply::Unhandled(); }
	virtual FReply OnMouseButtonUp(float X, float Y, int32 Button) { return FReply::Unhandled(); }
	virtual FReply OnMouseMove(float X, float Y) { return FReply::Unhandled(); }
	virtual FReply OnMouseWheel(float Delta) { return FReply::Unhandled(); }

	virtual void OnKeyDown(int32 Key) {}
	virtual void OnKeyUp(int32 Key) {}

	bool IsHovered(float X, float Y) const;

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
	virtual json::JSON Serialize();
	virtual void Deserialize(json::JSON& Data);

	bool IsRenderable() const { return bRenderable; }
	FColor GetColor() const { return Color; }
	void SetColor(const FColor& InColor) { Color = InColor; }

	virtual FRect GetRenderRect() { return Rect; }

protected:
	FString Name;

	FRect Rect;
	bool bIsVisible = true;

	SWidget* Parent = nullptr;
	TArray<SWidget*> Children;

	bool bRenderable = false; // 렌더링 가능한 위젯인지 여부 (기본값 false, SImage, SText 등에서 true로 설정)
	FColor Color = FColor::White(); // 위젯의 색상 (기본값 흰색)
};