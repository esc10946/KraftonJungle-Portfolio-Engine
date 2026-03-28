#pragma once

#include "Viewport/Viewport.h"
#include "Render/Resource/Shader.h"
#include "Render/Resource/VertexTypes.h"

class FViewportRenderer
{
public:
	FViewportRenderer() = default;
	~FViewportRenderer() = default;

public:
	void Create(ID3D11Device* InDevice);
	void Release();

	void Render(ID3D11DeviceContext* Context, FViewport* Viewport);

private:
	TArray<FTextureVertex> Vertices;
	TArray<uint32>		   Indices;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	ID3D11Device*	    Device = nullptr;
	ID3D11SamplerState* SamplerState = nullptr;
	FShader				ViewportShader;

	void CreateBuffers();
};
