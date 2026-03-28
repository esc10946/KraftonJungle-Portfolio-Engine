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
	// 1. 최상단 윈도우부터 거꾸로(Z-order 역순) 탐색
	for (auto It = Windows.rbegin(); It != Windows.rend(); ++It)
	{
		SWindow* Window = *It;
		if (Window->IsHovered(ScreenPoint)) // 마우스가 윈도우 영역 안인가?
		{
			// 2. 윈도우를 찾았으면, 그 내부의 자식들 중 가장 깊은 놈을 찾음
			return FindLeafWidgetAt(Window, ScreenPoint);
		}
	}
	return nullptr;
}

SWidget* FSlateApplication::FindLeafWidgetAt(SWidget* Parent, FVector2 ScreenPoint) const
{
	// 자식들을 역순으로 순회 (나중에 그려진 자식이 위에 있으므로)
	TArray<SWidget*> Children = Parent->GetChildren();
	for (int32 i = Children.size() - 1; i >= 0; --i)
	{
		SWidget* Child = Children[i];

		// 자식이 마우스를 무시하도록 설정되어 있다면 건너뜀 (HitTestInvisible)
		if (Child->IsHovered(ScreenPoint))
		{
			// 더 깊은 자식이 있는지 또 파고듬 (재귀)
			return FindLeafWidgetAt(Child, ScreenPoint);
		}
	}

	// 더 이상 마우스에 걸리는 자식이 없다면, 자기 자신(Parent)이 최하단 위젯임
	return Parent;
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

	int32 ButtonsToCheck[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
	for (int32 Button : ButtonsToCheck)
	{
		if (InputSystem::Get().GetKeyDown(Button))
		{
			SWidget* Target = Captor ? Captor : WidgetUnderMouse;

			if (Target)
			{
				FReply Reply = Target->OnMouseButtonDown(MousePos.X, MousePos.Y, Button);
				if (Reply.IsHandled())
				{
					MouseCaptor = Target;
				}
			}
		}

		if (InputSystem::Get().GetKeyUp(Button))
		{
			SWidget* Target = Captor ? Captor : WidgetUnderMouse;
			if (Target)
			{
				FReply Reply = Target->OnMouseButtonUp(MousePos.X, MousePos.Y, Button);
				if (Reply.IsHandled())
				{
					MouseCaptor = nullptr;
				}
			}
		}
	}
	
	// Mouse Wheel
	{
		SWidget* Target = Captor ? Captor : WidgetUnderMouse;

		if (Target)
		{
			FReply Reply = Target->OnMouseWheel(InputSystem::Get().GetScrollNotches());
		}
	}

	SWidget* KeyboardTarget = GetFocusedWidget();
	if (KeyboardTarget == nullptr)
	{
		KeyboardTarget = WidgetUnderMouse;
	}

	for (int32 Key = 0; Key < 256; ++Key)
	{
		if (InputSystem::Get().GetKeyDown(Key))
		{
			if (KeyboardTarget)
			{
				KeyboardTarget->OnKeyDown(Key);
			}
		}
		if (InputSystem::Get().GetKeyUp(Key))
		{
			if (KeyboardTarget)
			{
				KeyboardTarget->OnKeyUp(Key);
			}
		}
	}
}
