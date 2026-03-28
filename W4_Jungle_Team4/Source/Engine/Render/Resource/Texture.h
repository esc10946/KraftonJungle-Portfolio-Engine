#pragma once
#include "Object/Object.h"
#include <d3d11.h>
#include <wrl.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class UTexture :
    public UObject
{
public:
	DECLARE_CLASS(UTexture, UObject)
	
	UTexture(
		const ComPtr<ID3D11ShaderResourceView>& InSRV, 
		const ComPtr<ID3D11SamplerState>& InSampler, 
		uint32 InWidth = 0, 
		uint32 InHeight = 0) : 
			SRV(InSRV), Sampler(InSampler), Width(InWidth), Height(InHeight) {}
	
	ID3D11ShaderResourceView* GetSRV() const { return SRV.Get(); }
	ID3D11SamplerState* GetSampler() const { return Sampler.Get(); }

private:
	ComPtr<ID3D11ShaderResourceView> SRV;
	ComPtr<ID3D11SamplerState> Sampler;
	uint32 Width = 0;
	uint32 Height = 0;
	
	FName TextureFilePath;
	friend class FResourceManager;
};

