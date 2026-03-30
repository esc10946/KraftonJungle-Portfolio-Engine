#pragma once

#include "Core/EngineTypes.h"
#include "Core/Singleton.h"
#include "SWindow.h"
#include <functional>

class FSlateApplication : public TSingleton<FSlateApplication>
{
	friend class TSingleton<FSlateApplication>;

public:
	void Tick(float DeltaTime);
	void OnPaint();
	void OnWindowResized(uint32 Width, uint32 Height);

	SWindow* AddWindow(SWindow* InWindow);
	void DestroyWindow(SWindow* InWindow);
	TArray<SWindow*>& GetWindows() { return Windows; }

	template <typename T>
	TArray<T> GetWidgetsByType(SWidget* Root) const
	{
		TArray<T> Result;
		std::function<void(SWidget*)> FindWidgets = [&](SWidget* Widget)
			{
				if (!Widget) return;

				if (T TypedWidget = dynamic_cast<T>(Widget))
				{
					Result.push_back(TypedWidget);
				}
				for (SWidget* Child : Widget->GetChildren())
				{
					FindWidgets(Child);
				}
			};

		if (Root)
		{
			FindWidgets(Root);
		}

		return Result;
	}

	SWidget* GetMouseCaptor() const { return MouseCaptor; }

	SWidget* GetFocusedWidget() const { return FocusedWidget; }
	void SetFocusedWidget(SWidget* InWidget) { FocusedWidget = InWidget; }

private:
	SWidget* FindWidgetAt(FVector2 ScreenPoint) const;
	SWidget* FindLeafWidgetAt(SWidget* Parent, FVector2 ScreenPoint) const;

	void ProcessInput();

private:
	FSlateApplication() = default;
	~FSlateApplication();
	
	TArray<SWindow*> Windows;

	SWidget* MouseCaptor = nullptr;
	SWidget* WidgetUnderMouse = nullptr;
	SWidget* FocusedWidget = nullptr;
	SWidget* LastKeyboardTarget = nullptr;
};