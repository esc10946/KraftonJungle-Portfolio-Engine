#pragma once

#include "Core/EngineTypes.h"

class FViewportClient;
class ID3D11ShaderResourceView;

class FViewport
{
public:
	virtual ~FViewport() = default;

	virtual void Tick(float DeltaTime) = 0;

	virtual void Resize(uint32 InWidth, uint32 InHeight) = 0;

	void SetViewportClient(FViewportClient* InClient) { Client = InClient; }
	FViewportClient* GetViewportClient() const { return Client; }

	virtual ID3D11ShaderResourceView* GetRenderTargetTexture() const = 0;

private:
	FViewportClient* Client = nullptr;
	uint32 Width, Height;
};