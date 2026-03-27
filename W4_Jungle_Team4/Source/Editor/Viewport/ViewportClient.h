#pragma once

#include "Core/EngineTypes.h"

class FViewport;

class FViewportClient
{
public:
	virtual ~FViewportClient() = default;

	virtual void Tick(float DeltaTime) = 0;

	virtual void OnViewportResized(uint32 NewWidth, uint32 NewHeight) = 0;
};