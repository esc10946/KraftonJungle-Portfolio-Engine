#include "SlateBatcher.h"
#include "Editor/EditorEngine.h"
#include "Runtime/WindowsWindow.h"

namespace
{
	void ReleaseBuffer(ID3D11Buffer*& Buffer)
	{
		if (Buffer)
		{
			Buffer->Release();
			Buffer = nullptr;
		}
	}

	bool CreateDynamicBuffer(ID3D11Device* Device, uint32 ByteWidth, UINT BindFlags, ID3D11Buffer** OutBuffer)
	{
		if (!Device || ByteWidth == 0 || !OutBuffer)
		{
			return false;
		}

		D3D11_BUFFER_DESC Desc = {};
		Desc.ByteWidth = ByteWidth;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = BindFlags;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		return SUCCEEDED(Device->CreateBuffer(&Desc, nullptr, OutBuffer));
	}
}

extern UEngine* GEngine; // TODO: Window 크기를 얻기 위해 참조했는데 수정 필요할듯

void FSlateBatcher::Create(ID3D11Device* InDevice)
{
	Device = InDevice;
	if (!Device) return;

	Device->AddRef();

	MaxIndexedVertexCount = 512;
	MaxIndexCount = 1536;

	if (!CreateDynamicBuffer(Device, sizeof(FRect2DVertex) * MaxIndexedVertexCount, D3D11_BIND_VERTEX_BUFFER, &IndexedVertexBuffer) ||
		!CreateDynamicBuffer(Device, sizeof(uint32) * MaxIndexCount, D3D11_BIND_INDEX_BUFFER, &IndexBuffer))
	{
		Release();
		return;
	}

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FRect2DVertex, Color)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	ScreenQuadShader.Create(Device, L"Shaders/ScreenQuadSimple.hlsl", "VS", "PS", layout, ARRAYSIZE(layout));
}

void FSlateBatcher::Release()
{
	ReleaseBuffer(IndexedVertexBuffer);
	ReleaseBuffer(IndexBuffer);

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}

	MaxIndexedVertexCount = 0;
	MaxIndexCount = 0;
	IndexedVertices.clear();
	Indices.clear();

	ScreenQuadShader.Release();
}

void FSlateBatcher::AddRect(const FRect& ScreenRect, const FVector4& Color)
{
	if (IndexedVertices.size() + 4 > MaxIndexedVertexCount ||
		Indices.size() + 6 > MaxIndexCount)
	{
		return; // 버퍼 용량 초과 시 추가 무시
	}

	uint32 StartIndex = static_cast<uint32>(IndexedVertices.size());
	IndexedVertices.emplace_back(FVector(ScreenRect.X, ScreenRect.Y, 0.0f), Color);
	IndexedVertices.emplace_back(FVector(ScreenRect.X + ScreenRect.Width, ScreenRect.Y, 0.0f), Color);
	IndexedVertices.emplace_back(FVector(ScreenRect.X, ScreenRect.Y + ScreenRect.Height, 0.0f), Color);
	IndexedVertices.emplace_back(FVector(ScreenRect.X + ScreenRect.Width, ScreenRect.Y + ScreenRect.Height, 0.0f), Color);
	Indices.push_back(StartIndex);
	Indices.push_back(StartIndex + 1);
	Indices.push_back(StartIndex + 2);
	Indices.push_back(StartIndex + 1);
	Indices.push_back(StartIndex + 3);
	Indices.push_back(StartIndex + 2);
}

void FSlateBatcher::Clear()
{
	IndexedVertices.clear();
	Indices.clear();
}

void FSlateBatcher::Flush(ID3D11DeviceContext* Context)
{
	if (!Context || !Device) return;

	const uint32 RequiredIndexedVertexCount = static_cast<uint32>(IndexedVertices.size());
	const uint32 RequiredIndexCount = static_cast<uint32>(Indices.size());
	if (RequiredIndexedVertexCount == 0 || RequiredIndexCount == 0) return;

	if (!IndexedVertexBuffer || RequiredIndexedVertexCount > MaxIndexedVertexCount)
	{
		ReleaseBuffer(IndexedVertexBuffer);
		MaxIndexedVertexCount = RequiredIndexedVertexCount * 2;
		if (!CreateDynamicBuffer(Device, sizeof(FRect2DVertex) * MaxIndexedVertexCount, D3D11_BIND_VERTEX_BUFFER, &IndexedVertexBuffer))
		{
			MaxIndexedVertexCount = 0;
			return;
		}
	}

	if (!IndexBuffer || RequiredIndexCount > MaxIndexCount)
	{
		ReleaseBuffer(IndexBuffer);
		MaxIndexCount = RequiredIndexCount * 2;
		if (!CreateDynamicBuffer(Device, sizeof(uint32) * MaxIndexCount, D3D11_BIND_INDEX_BUFFER, &IndexBuffer))
		{
			MaxIndexCount = 0;
			return;
		}
	}

	float ScreenWidth = GEngine->GetWindow()->GetWidth();
	float ScreenHeight = GEngine->GetWindow()->GetHeight();

	for (auto& V : IndexedVertices)
	{
		V.Position.X = (V.Position.X / ScreenWidth) * 2.0f - 1.0f;
		V.Position.Y = 1.0f - (V.Position.Y / ScreenHeight) * 2.0f;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	if (FAILED(Context->Map(IndexedVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
	{
		return;
	}

	memcpy(MappedResource.pData, IndexedVertices.data(), sizeof(FRect2DVertex) * RequiredIndexedVertexCount);
	Context->Unmap(IndexedVertexBuffer, 0);

	if (FAILED(Context->Map(IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
	{
		return;
	}

	memcpy(MappedResource.pData, Indices.data(), sizeof(uint32) * RequiredIndexCount);
	Context->Unmap(IndexBuffer, 0);

	ScreenQuadShader.Bind(Context);

	UINT Stride = sizeof(FRect2DVertex);
	UINT Offset = 0;
	Context->IASetVertexBuffers(0, 1, &IndexedVertexBuffer, &Stride, &Offset);
	Context->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Context->DrawIndexed(RequiredIndexCount, 0, 0);
}
