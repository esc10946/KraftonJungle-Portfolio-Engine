#pragma once
#include "SWidget.h"
#include "Core/EngineTypes.h"

class SViewport;

class SViewportRoot : public SWidget
{
public:
	SViewportRoot();
	virtual ~SViewportRoot() override;

public:
	void Tick(float DeltaTime) override;

	void Construct();
	void SetVisibleCount(int32 Count)
	{
		VisibleCount = Count;
		bDirty = true;
	}

	FString GetWidgetType() const override { return "SViewportRoot"; }

	json::JSON Serialize() const override;
	void Deserialize(json::JSON& Data) override;

private:
	TArray<SViewport*> Viewports;

	bool bDirty = true;
	int32 VisibleCount = 1;
};

