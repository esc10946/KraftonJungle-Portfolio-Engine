#pragma once

#include "Core/EngineTypes.h"
#include <d3d11.h>

struct FRenderTarget
{
	ID3D11Texture2D* Texture = nullptr;
	ID3D11RenderTargetView* RTV = nullptr;
	ID3D11ShaderResourceView* SRV = nullptr;

	ID3D11Texture2D* DepthStencilTexture = nullptr;
	ID3D11DepthStencilView* DSV = nullptr;

	uint32 Width = 0;
	uint32 Height = 0;

	void Release()
	{
		if (DSV) DSV->Release();
		if (DepthStencilTexture) DepthStencilTexture->Release();

		if (SRV) SRV->Release();
		if (RTV) RTV->Release();
		if (Texture) Texture->Release();
	}
};