#include "ViewportRenderer.h"

void FViewportRenderer::Render(ID3D11DeviceContext* Context, FViewport* Viewport)
{
	ViewportShader.Bind(Context);
	Context->PSSetSamplers(0, 1, &SamplerState);

	UINT stride = sizeof(FTextureVertex);
	UINT offset = 0;
	Context->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	Context->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 뷰포트 텍스처 SRV는 Viewport가 소유하므로 전달받아서 사용
	Context->PSSetShaderResources(0, 1, &Viewport->GetRenderTarget().SRV);

	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = static_cast<float>(Viewport->GetSize().X);
	vp.Height = static_cast<float>(Viewport->GetSize().Y);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	Context->RSSetViewports(1, &vp);

	Context->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	Context->PSSetShaderResources(0, 1, nullSRV);
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

	// 셰이더 + Input Layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	ViewportShader.Create(Device, L"Shaders/ShaderQuad.hlsl",
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
	if (SamplerState) { SamplerState->Release(); SamplerState = nullptr; }
	ViewportShader.Release();
}
