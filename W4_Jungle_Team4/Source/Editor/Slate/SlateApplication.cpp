#include "SlateApplication.h"
#include "Core/InputSystem.h"
#include "SSplitter.h"
#include <functional>

FSlateApplication::~FSlateApplication()
{
	for (SWindow* Window : Windows)
	{
		delete Window;
	}
	Windows.clear();
}

void FSlateApplication::Tick(float DeltaTime)
{
	ProcessInput();
	for (SWindow* Window : Windows)
	{
		Window->Tick(DeltaTime);
	}
}

void FSlateApplication::OnPaint()
{
	for (SWindow* Window : Windows)
	{
		Window->OnPaint();
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


// 마우스 위치에 있는 최하단 위젯을 찾는 함수
SWidget* FSlateApplication::FindWidgetAt(FVector2 ScreenPoint) const
{
	for (auto It = Windows.rbegin(); It != Windows.rend(); ++It)
	{
		SWindow* Window = *It;
		if (Window->IsHovered(ScreenPoint))
		{
			return FindLeafWidgetAt(Window, ScreenPoint);
		}
	}
	return nullptr;
}

SWidget* FSlateApplication::FindLeafWidgetAt(SWidget* Parent, FVector2 ScreenPoint) const
{
	// SSplitter인 경우 자식들을 검사하기 전에 본인의 핸들을 먼저 검사 (예외 처리)
	if (SSplitter* Splitter = dynamic_cast<SSplitter*>(Parent))
	{
		if (Splitter->IsOnHandle(ScreenPoint.X, ScreenPoint.Y))
		{
			return Splitter;
		}
	}	

	TArray<SWidget*> Children = Parent->GetChildren();
	for (int32 i = Children.size() - 1; i >= 0; --i)
	{
		SWidget* Child = Children[i];

		// TODO: 현재는 모든 child를 순회하지만, 따로 pass flag를 둘 수 있음
		if (Child->IsHovered(ScreenPoint))
		{
			// 더 깊은 자식이 있는지 또 파고듬 (재귀)
			return FindLeafWidgetAt(Child, ScreenPoint);
		}
	}

	return Parent;
}

// 모든 입력을 처리하는 함수
// 마우스 위치, 버튼 상태, 키보드 입력 등을 체크해서 적절한 위젯에 이벤트를 전달
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
	SWidget* WheelTarget = Captor ? Captor : WidgetUnderMouse;

	if (WheelTarget)
	{
		FReply Reply = WheelTarget->OnMouseWheel(InputSystem::Get().GetScrollNotches());
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
