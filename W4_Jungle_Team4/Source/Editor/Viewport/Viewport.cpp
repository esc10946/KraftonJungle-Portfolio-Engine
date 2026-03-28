#include "Viewport.h"

#include "Render/Device/D3DDevice.h"
#include "ViewportClient.h"

FViewport::FViewport(FD3DDevice* Device)
	: Device(Device), RenderTarget()
{
	Size = { 400, 300 };
	RenderTarget = Device->CreateRenderTarget(0, 0, Size.X, Size.Y);
}

void FViewport::Resize(uint32 InLeftTopX, uint32 InLeftTopY, uint32 InWidth, uint32 InHeight)
{
	Pos = FVector2(InLeftTopX, InLeftTopY);

	if (InWidth == Size.X && InHeight == Size.Y) return;

	Size = FVector2(InWidth, InHeight);

	RenderTarget.Release();

	RenderTarget = Device->CreateRenderTarget(InLeftTopX, InLeftTopY, InWidth, InHeight);
}
