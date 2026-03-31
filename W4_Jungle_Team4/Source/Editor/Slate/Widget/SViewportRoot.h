#pragma once
#include "SWidget.h"
#include "Core/EngineTypes.h"

class SViewport;

enum class EViewportLayout : int32
{
	Single = 0,
	TwoHorizontal,
	TwoVertical,
	Four,
	All
};

struct FViewportLayoutInfo
{
	EViewportLayout Layout;
	TArray<float> SplitRatios;
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
			SaveCurrentLayoutInfo();

			CurrentLayout = NewLayout;
			bDirty = true;
		}
	}

	static FString LayoutToName(EViewportLayout Layout);
	void SaveCurrentLayoutInfo();

	FString GetWidgetType() const override { return "SViewportRoot"; }

	json::JSON Serialize() override;
	void Deserialize(json::JSON& Data) override;

private:
	TArray<SViewport*> Viewports;

	bool bDirty = true;
	EViewportLayout CurrentLayout = EViewportLayout::Single;

	FViewportLayoutInfo LayoutInfos[static_cast<int32>(EViewportLayout::All)] = {
		{ EViewportLayout::Single, {} },
		{ EViewportLayout::TwoHorizontal, {0.5f} },
		{ EViewportLayout::TwoVertical, {0.5f} },
		{ EViewportLayout::Four, {0.5f, 0.5f} }
	};
};

