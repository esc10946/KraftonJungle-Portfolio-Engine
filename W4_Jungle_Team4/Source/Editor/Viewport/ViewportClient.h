#pragma once

#include "Core/EngineTypes.h"

class FViewport;

class FViewportClient
{
public:
	virtual ~FViewportClient() = default;

	virtual void Tick(float DeltaTime) = 0;

	virtual void OnViewportResized(uint32 NewTopLeftX, uint32 NewTopLeftY, uint32 NewWidth, uint32 NewHeight) = 0;

	virtual void OnMouseDown(float X, float Y, int32 Button) = 0;
	virtual void OnMouseUp(float X, float Y, int32 Button) = 0;
	virtual void OnMouseMove(float X, float Y) = 0;
	virtual void OnMouseWheel(float Delta) = 0;

	virtual void OnKeyDown(int32 Key) = 0;
	virtual void OnKeyUp(int32 Key) = 0;
};