#pragma once

#include "SWidget.h"
#include "Core/EngineTypes.h"

class SWindow : public SWidget
{
public:
	SWindow() : Title("New Window"), bIsActive(false) {}
	virtual ~SWindow() = default;

	void SetTitle(const FString& InTitle) { Title = InTitle; }
	const FString& GetTitle() const { return Title; }

private:
	FString Title;
	bool bIsActive;
};