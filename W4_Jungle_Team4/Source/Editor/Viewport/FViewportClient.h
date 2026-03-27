#pragma once

#include "Core/EngineTypes.h"

class FViewport;

class FViewportClient
{
public:
	virtual ~FViewportClient() = default;

	virtual void Tick(float DeltaTime) = 0;

	virtual void Draw(FViewport* Viewport, struct ID3D11DeviceContext* DeviceContext) = 0;

	virtual void OnViewportResized(FViewport* Viewport, uint32 NewWidth, uint32 NewHeight) = 0;
};