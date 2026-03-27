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
		}

		ResizeViewport();
	}
}

FReply SViewport::OnMouseButtonDown(float X, float Y)
{
	FReply Reply = SWidget::OnMouseButtonDown(X, Y);
	if (Reply.IsHandled())
	{
		return Reply;
	}
	if (Viewport)
	{
		// Viewport의 마우스 입력 처리 로직을 여기에 추가
	}
	return FReply::Handled().CaptureMouse(this);
}

FReply SViewport::OnMouseButtonUp(float X, float Y)
{
	FReply Reply = SWidget::OnMouseButtonUp(X, Y);
	if (Reply.IsHandled())
	{
		return Reply;
	}
	if (Viewport)
	{
		// Viewport의 마우스 입력 처리 로직을 여기에 추가
	}
	return FReply::Handled();
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
		// Viewport의 마우스 입력 처리 로직을 여기에 추가
	}
	return FReply::Handled();
}

void SViewport::ResizeViewport()
{
	if (Viewport)
	{
		FViewportClient* Client = Viewport->GetViewportClient();
		if (Client)
		{
			Client->OnViewportResized(Rect.Width, Rect.Height);
		}
	}
}
