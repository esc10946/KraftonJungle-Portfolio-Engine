#pragma once
#include "SWidget.h"
#include "Core/EngineTypes.h"

class SViewport;

enum class EViewportLayout : int32
{
	Single = 0,
	TwoHorizontal,
	TwoVertical,
	Four
};

class SViewportRoot : public SWidget
{
public:
	SViewportRoot();
	virtual ~SViewportRoot() override;

public:
	void Tick(float DeltaTime) override;

	void Construct();
	void SetLayout(EViewportLayout NewLayout)
	{
		if (CurrentLayout != NewLayout)
		{
			CurrentLayout = NewLayout;
			bDirty = true;
		}
	}

	static FString LayoutToName(EViewportLayout Layout);

	FString GetWidgetType() const override { return "SViewportRoot"; }

	json::JSON Serialize() const override;
	void Deserialize(json::JSON& Data) override;

private:
	TArray<SViewport*> Viewports;

	bool bDirty = true;
	EViewportLayout CurrentLayout = EViewportLayout::Single;
};

