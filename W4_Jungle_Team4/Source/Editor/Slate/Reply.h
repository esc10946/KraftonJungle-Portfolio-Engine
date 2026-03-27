#pragma once

class SWidget;

struct FReply
{
public:
	static FReply Handled() { return FReply(true); }
	static FReply Unhandled() { return FReply(false); }

	FReply& CaptureMouse(SWidget* InWidget)
	{
		MouseCaptor = InWidget;
		return *this;
	}

	bool IsHandled() const { return bIsHandled; }
	SWidget* GetMouseCaptor() const { return MouseCaptor; }

private:
	FReply(bool bInHandled) : bIsHandled(bInHandled), MouseCaptor(nullptr) {}

	bool bIsHandled;
	SWidget* MouseCaptor;
};
