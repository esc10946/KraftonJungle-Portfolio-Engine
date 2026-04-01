#pragma once
#include "SWidget.h"
#include "Core/EngineTypes.h"

class SViewport;

enum class EViewportLayout : int32
{
	Single = 0,

	TwoHorizontal,
	TwoVertical,

	ThreeLeftMain,
	ThreeRightMain,
	ThreeTopMain,
	ThreeBottomMain,

	FourQuad,
	FourLeftMain,
	FourRightMain,
	FourTopMain,
	FourBottomMain,
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
	EViewportLayout GetLayout() const { return CurrentLayout; }

	static FString LayoutToName(EViewportLayout Layout);
	void SaveCurrentLayoutInfo();

	FString GetWidgetType() const override { return "SViewportRoot"; }

	json::JSON Serialize() override;
	void Deserialize(json::JSON& Data) override;

private:
	bool bDirty = true;
	EViewportLayout CurrentLayout = EViewportLayout::Single;

	FViewportLayoutInfo LayoutInfos[static_cast<int32>(EViewportLayout::All)] = {
		{ EViewportLayout::Single, {} },
		{ EViewportLayout::TwoHorizontal, {0.5f} },
		{ EViewportLayout::TwoVertical, {0.5f} },
		{ EViewportLayout::ThreeTopMain, {0.5f, 0.5f} },
		{ EViewportLayout::ThreeBottomMain, {0.5f, 0.5f} },
		{ EViewportLayout::ThreeLeftMain, {0.5f, 0.5f} },
		{ EViewportLayout::ThreeRightMain, {0.5f, 0.5f} },
		{ EViewportLayout::FourQuad, {0.5f, 0.5f, 0.5f} },
		{ EViewportLayout::FourLeftMain, {0.66f, 0.33f, 0.5f} },
		{ EViewportLayout::FourRightMain, {0.33f, 0.33f, 0.5f} },
		{ EViewportLayout::FourTopMain, {0.66f, 0.33f, 0.5f} },
		{ EViewportLayout::FourBottomMain, {0.33f, 0.33f, 0.5f} }
	};
};

