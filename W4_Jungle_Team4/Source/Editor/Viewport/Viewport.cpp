#include "Viewport.h"

#include "Render/Device/D3DDevice.h"
#include "ViewportClient.h"

FViewport::FViewport(FD3DDevice* Device)
	: Device(Device), RenderTarget()
{
	Size = { 800, 600 };
	RenderTarget = Device->CreateRenderTarget(Size.X, Size.Y);
}

void FViewport::Resize(uint32 InLeftTopX, uint32 InLeftTopY, uint32 InWidth, uint32 InHeight)
{
	if (InWidth == Size.X && InHeight == Size.Y) return;

	Pos = FVector2(InLeftTopX, InLeftTopY);
	Size = FVector2(InWidth, InHeight);

	RenderTarget.Release();

	RenderTarget = Device->CreateRenderTarget(InWidth, InHeight);

	if (Client)
	{
		Client->OnViewportResized(InWidth, InHeight);
	}
}
