#pragma once

#include "Core/EngineTypes.h"

class FViewportClient;
struct ID3D11ShaderResourceView;

class FViewport
{
public:
	virtual ~FViewport() = default;

	virtual void Tick(float DeltaTime) = 0;

	virtual void Resize(uint32 InWidth, uint32 InHeight) = 0;

	void SetViewportClient(FViewportClient* InClient) { Client = InClient; }
	FViewportClient* GetViewportClient() const { return Client; }
	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }

	virtual ID3D11ShaderResourceView* GetRenderTargetTexture() const = 0;

protected:
	void SetViewportSizeInternal(uint32 InWidth, uint32 InHeight)
	{
		Width = InWidth;
		Height = InHeight;
	}

private:
	FViewportClient* Client = nullptr;
	uint32 Width = 0;
	uint32 Height = 0;
};
