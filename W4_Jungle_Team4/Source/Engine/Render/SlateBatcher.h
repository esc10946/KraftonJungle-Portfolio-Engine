#pragma once

#include "Core/CoreTypes.h"
#include "Slate/SWidget.h"
#include "Render/Common/RenderTypes.h"
#include "Render/Resource/Shader.h"

struct FRect2DVertex
{
	FVector Position;
	FVector4 Color;

	FRect2DVertex(const FVector& InPos, const FVector4& InColor) : Position(InPos), Color(InColor) {}
};

class FSlateBatcher
{
public:
	FSlateBatcher() = default;
	~FSlateBatcher() = default;

	void Create(ID3D11Device* Device);
	void Release();

	void AddRect(const FRect& ScreenRect, const FVector4& Color);

	void Clear();

	void Flush(ID3D11DeviceContext* Context);

private:
	TArray<FRect2DVertex> IndexedVertices;
	TArray<uint32> Indices;

	ID3D11Buffer* IndexedVertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	ID3D11Device* Device = nullptr;
	uint32 MaxIndexedVertexCount = 0;
	uint32 MaxIndexCount = 0;

	FShader ScreenQuadShader;
};