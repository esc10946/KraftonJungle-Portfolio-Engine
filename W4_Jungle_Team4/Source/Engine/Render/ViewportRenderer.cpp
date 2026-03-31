#include "ViewportRenderer.h"
#include <cstring>

void FViewportRenderer::Render(ID3D11DeviceContext* Context, FViewport* Viewport)
{
	ViewportCompositeShader.Bind(Context);
	Context->PSSetSamplers(0, 1, &SamplerState);

	FOutlineCompositeConstants Constants = {};
	Constants.CompositeOutlineColor = FVector4(1.0f, 0.5f, 0.0f, 1.0f);
	Constants.CompositeTexelSize = FVector2(
		1.0f / static_cast<float>(Viewport->GetSize().X),
		1.0f / static_cast<float>(Viewport->GetSize().Y));
	Constants.CompositeOutlineThickness = 2.0f;
	Constants.CompositePadding = 0.0f;

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	Context->Map(OutlineCompositeConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	std::memcpy(Mapped.pData, &Constants, sizeof(Constants));
	Context->Unmap(OutlineCompositeConstantBuffer, 0);

	Context->PSSetConstantBuffers(0, 1, &OutlineCompositeConstantBuffer);

	UINT stride = sizeof(FTextureVertex);
	UINT offset = 0;
	Context->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	Context->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11ShaderResourceView* SRVs[2] =
	{
		Viewport->GetRenderTarget().SRV,
		Viewport->GetSelectionMaskRenderTarget().SRV
	};
	Context->PSSetShaderResources(0, 2, SRVs);

	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = Viewport->GetPos().X;
	vp.TopLeftY = Viewport->GetPos().Y;
	vp.Width = static_cast<float>(Viewport->GetSize().X);
	vp.Height = static_cast<float>(Viewport->GetSize().Y);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	Context->RSSetViewports(1, &vp);
	Context->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* NullSRVs[2] = { nullptr, nullptr };
	Context->PSSetShaderResources(0, 2, NullSRVs);
}

void FViewportRenderer::Create(ID3D11Device* InDevice)
{
	Device = InDevice;

	CreateBuffers();

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	Device->CreateSamplerState(&sampDesc, &SamplerState);

	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth = sizeof(FOutlineCompositeConstants);
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Device->CreateBuffer(&cbDesc, nullptr, &OutlineCompositeConstantBuffer);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, static_cast<UINT>(offsetof(FTextureVertex, TexCoord)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	ViewportCompositeShader.Create(Device, L"Shaders/ViewportOutlineComposite.hlsl",
		"VS", "PS", layout, ARRAYSIZE(layout));
}

void FViewportRenderer::CreateBuffers()
{
	if (VertexBuffer) { VertexBuffer->Release(); VertexBuffer = nullptr; }
	if (IndexBuffer) { IndexBuffer->Release();  IndexBuffer = nullptr; }

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(FTextureVertex) * 4;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	FTextureVertex vertices[] =
	{
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } },
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
	};
	D3D11_SUBRESOURCE_DATA initData = { vertices };
	Device->CreateBuffer(&vbDesc, &initData, &VertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32) * 6;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	uint32 indices[] = { 0, 1, 2, 2, 1, 3 };
	D3D11_SUBRESOURCE_DATA initIndexData = { indices };
	Device->CreateBuffer(&ibDesc, &initIndexData, &IndexBuffer);
}

void FViewportRenderer::Release()
{
	if (VertexBuffer) { VertexBuffer->Release(); VertexBuffer = nullptr; }
	if (IndexBuffer) { IndexBuffer->Release();  IndexBuffer = nullptr; }
	if (OutlineCompositeConstantBuffer) { OutlineCompositeConstantBuffer->Release(); OutlineCompositeConstantBuffer = nullptr; }
	if (SamplerState) { SamplerState->Release(); SamplerState = nullptr; }
	ViewportCompositeShader.Release();
}