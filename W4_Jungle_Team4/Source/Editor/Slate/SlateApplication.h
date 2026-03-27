#pragma once

#include "Core/EngineTypes.h"
#include "Core/Singleton.h"
#include "SWindow.h"

class FSlateApplication : public TSingleton<FSlateApplication>
{
	friend class TSingleton<FSlateApplication>;

public:
	void Tick(float DeltaTime);

	SWindow* AddWindow(SWindow* InWindow);
	void DestroyWindow(SWindow* InWindow);
	TArray<SWindow*>& GetWindows() { return Windows; }

	SWidget* GetMouseCaptor() const { return MouseCaptor; }

private:
	SWidget* FindWidgetAt(FVector2 ScreenPoint) const;

	void ProcessInput();

private:
	FSlateApplication() = default;
	~FSlateApplication() = default;
	
	TArray<SWindow*> Windows;

	SWidget* MouseCaptor = nullptr;
	SWidget* WidgetUnderMouse = nullptr;
};