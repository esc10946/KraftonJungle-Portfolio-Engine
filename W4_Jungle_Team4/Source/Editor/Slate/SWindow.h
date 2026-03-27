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

	void SetContent(SWidget* InContent) { Content = InContent; }
	SWidget* GetContent() const { return Content; }

	virtual void Tick(float DeltaTime) override
	{
		if (Content)
		{
			Content->Tick(DeltaTime);
		}
	}

private:
	FString Title;
	SWidget* Content = nullptr;
	bool bIsActive;
};