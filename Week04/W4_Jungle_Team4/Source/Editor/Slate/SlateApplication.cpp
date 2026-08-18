#include "SlateApplication.h"
#include "Core/InputSystem.h"
#include "Widget/SSplitter.h"
#include "ImGui/imgui.h"
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

void FSlateApplication::OnWindowResized(uint32 Width, uint32 Height)
{
	for (SWindow* Window : Windows)
	{
		FRect NewRect = { 0, 0, Width, Height };
		Window->OnResize(NewRect);
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
		if (Window->IsHovered(ScreenPoint.X, ScreenPoint.Y))
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
		if (Child->IsHovered(ScreenPoint.X, ScreenPoint.Y))
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
	POINT MousePosition = InputSystem::Get().GetClientMousePos();
	FVector2 MousePos = { static_cast<float>(MousePosition.x), static_cast<float>(MousePosition.y) };

	ImGuiIO& io = ImGui::GetIO();

	SWidget* Captor = GetMouseCaptor();
	if (!Captor)
	{
		SWidget* HoveredWidget = FindWidgetAt(MousePos);
		Captor = HoveredWidget;
		WidgetUnderMouse = HoveredWidget;
	}

	if (Captor) Captor->OnMouseMove(MousePos.X, MousePos.Y);

	int32 ButtonsToCheck[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
	for (int32 Button : ButtonsToCheck)
	{
		if (InputSystem::Get().GetKeyDown(Button))
		{
			
			FReply Reply = FReply::Unhandled();
				
			if (Captor) Reply = Captor->OnMouseButtonDown(MousePos.X, MousePos.Y, Button);
			if (Reply.IsHandled())
			{
				MouseCaptor = Captor;
			}
		}

		if (InputSystem::Get().GetKeyUp(Button))
		{
			FReply Reply = FReply::Unhandled();

			if (Captor) Reply = Captor->OnMouseButtonUp(MousePos.X, MousePos.Y, Button);
			if (Reply.IsHandled())
			{
				MouseCaptor = nullptr;
			}
		}
	}
	
	FReply Reply = FReply::Unhandled();
	if (Captor) Reply = Captor->OnMouseWheel(InputSystem::Get().GetScrollNotches());

	// KeyboardTarget이 바뀌었다면, 이전 타켓에게 눌려있던 키들의 Up 이벤트를 보내줌
	if (LastKeyboardTarget && LastKeyboardTarget != Captor)
	{
		for (int32 Key = 0; Key < 256; ++Key)
		{
			if (InputSystem::Get().GetKey(Key))
			{
				LastKeyboardTarget->OnKeyUp(Key);
			}
		}
	}
	LastKeyboardTarget = Captor;

	for (int32 Key = 0; Key < 256; ++Key)
	{
		if (InputSystem::Get().GetKeyDown(Key) && !io.WantCaptureKeyboard)
		{
			if (Captor) Captor->OnKeyDown(Key);
		}
		if (InputSystem::Get().GetKeyUp(Key) && !io.WantCaptureKeyboard)
		{
			if (Captor) Captor->OnKeyUp(Key);
		}
	}
}
