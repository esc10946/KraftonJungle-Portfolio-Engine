#pragma once

#include "Core/EngineTypes.h"
#include "Core/Singleton.h"
#include "SWindow.h"

class FSlateApplication : public TSingleton<FSlateApplication>
{
	friend class TSingleton<FSlateApplication>;

public:
	void Tick(float DeltaTime);
	void Draw();

	SWindow* AddWindow(SWindow* InWindow);
	void DestroyWindow(SWindow* InWindow);

	SWidget* GetMouseCaptor() const { return MouseCaptor; }

private:
	SWidget* FindWidgetAt(FVector2 ScreenPosition) const;

	void ProcessInput();

private:
	FSlateApplication() = default;
	~FSlateApplication() = default;
	
	TArray<SWindow*> Windows;

	SWidget* MouseCaptor = nullptr;
	SWidget* WidgetUnderMouse = nullptr;
};