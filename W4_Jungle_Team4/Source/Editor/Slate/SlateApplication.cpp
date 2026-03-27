#include "SlateApplication.h"
#include "Core/InputSystem.h"

void FSlateApplication::Tick(float DeltaTime)
{
	ProcessInput();
	for (SWindow* Window : Windows)
	{
		Window->Tick(DeltaTime);
	}
}

SWindow* FSlateApplication::AddWindow(SWindow* InWindow)
{
	Windows.push_back(InWindow);
	return InWindow;
}

void FSlateApplication::DestroyWindow(SWindow* InWindow)
{
	auto It = std::find(Windows.begin(), Windows.end(), InWindow);
	if (It != Windows.end())
	{
		Windows.erase(It);
		delete InWindow;
	}
}

SWidget* FSlateApplication::FindWidgetAt(FVector2 ScreenPoint) const
{
	for (auto It = Windows.rbegin(); It != Windows.rend(); ++It)
	{
		SWindow* Window = *It;
		if (Window->IsHovered(ScreenPoint))
		{
			return Window;
		}
	}
	return nullptr;
}

void FSlateApplication::ProcessInput()
{
	POINT MousePosition = InputSystem::Get().GetMousePos();
	FVector2 MousePos = { static_cast<float>(MousePosition.x), static_cast<float>(MousePosition.y) };

	SWidget* Captor = GetMouseCaptor();
	if (Captor)
	{
		Captor->OnMouseMove(MousePos.X, MousePos.Y);
	}
	else
	{
		SWidget* HoveredWidget = FindWidgetAt(MousePos);
		if (HoveredWidget != WidgetUnderMouse)
		{
			if (WidgetUnderMouse)
			{
				// Handle mouse leave for the previous widget if necessary.
			}
			if (HoveredWidget)
			{
				// Handle mouse enter for the new widget if necessary.
			}
			WidgetUnderMouse = HoveredWidget;
		}
	}
}
