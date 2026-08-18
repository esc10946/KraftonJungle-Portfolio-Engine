#pragma once

#include "Viewport/Viewport.h"
#include "Render/Resource/Shader.h"
#include "Render/Resource/VertexTypes.h"

struct FOutlineCompositeConstants
{
	FVector4 CompositeOutlineColor = FVector4(1.0f, 0.5f, 0.0f, 1.0f);
	FVector2 CompositeTexelSize = FVector2(0.0f, 0.0f);
	float CompositeOutlineThickness = 1.0f;
	float CompositePadding = 0.0f;
};

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
	ID3D11Buffer* OutlineCompositeConstantBuffer = nullptr;

	ID3D11Device*	    Device = nullptr;
	ID3D11SamplerState* SamplerState = nullptr;
	FShader				ViewportCompositeShader;

	void CreateBuffers();
};
