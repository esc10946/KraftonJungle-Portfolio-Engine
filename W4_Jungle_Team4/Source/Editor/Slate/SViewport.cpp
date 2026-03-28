#include "SViewport.h"
#include "Viewport/Viewport.h"
#include "Viewport/ViewportClient.h"

void SViewport::Tick(float DeltaTime)
{
	if (Viewport)
	{
		FViewportClient* Client = Viewport->GetViewportClient();
		if (Client)
		{
			Client->Tick(DeltaTime);
			Client->OnViewportResized(Rect.X, Rect.Y, Rect.Width, Rect.Height);
		}
	}
}

FReply SViewport::OnMouseButtonDown(float X, float Y, int32 Button)
{
	FReply Reply = SWidget::OnMouseButtonUp(X, Y, Button);
	if (Reply.IsHandled())
	{
		return Reply;
	}

	if (Viewport)
	{
		if (Viewport->GetViewportClient())
		{
			Viewport->GetViewportClient()->OnMouseDown(X, Y, Button);
		}
		return FReply::Handled().CaptureMouse(this);
	}

	return FReply::Unhandled();
}

FReply SViewport::OnMouseButtonUp(float X, float Y, int32 Button)
{
	FReply Reply = SWidget::OnMouseButtonUp(X, Y, Button);
	if (Reply.IsHandled())
	{
		return Reply;
	}

	if (Viewport)
	{
		if (Viewport->GetViewportClient())
		{
			Viewport->GetViewportClient()->OnMouseUp(X, Y, Button);
		}
		return FReply::Handled().CaptureMouse(nullptr);
	}

	return FReply::Unhandled();
}

FReply SViewport::OnMouseMove(float X, float Y)
{
	FReply Reply = SWidget::OnMouseMove(X, Y);
	if (Reply.IsHandled())
	{
		return Reply;
	}

	if (Viewport)
	{
		if (Viewport->GetViewportClient())
		{
			Viewport->GetViewportClient()->OnMouseMove(X, Y);
		}
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

FReply SViewport::OnMouseWheel(float Delta)
{
	FReply Reply = SWidget::OnMouseWheel(Delta);
	if (Reply.IsHandled())
	{
		return Reply;
	}
	if (Viewport)
	{
		if (Viewport->GetViewportClient())
		{
			Viewport->GetViewportClient()->OnMouseWheel(Delta);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SViewport::OnKeyDown(int32 Key)
{
	SWidget::OnKeyDown(Key);
	if (Viewport && Viewport->GetViewportClient())
	{
		Viewport->GetViewportClient()->OnKeyDown(Key);
	}
}

void SViewport::OnKeyUp(int32 Key)
{
	SWidget::OnKeyUp(Key);
	if (Viewport && Viewport->GetViewportClient())
	{
		Viewport->GetViewportClient()->OnKeyUp(Key);
	}
}
