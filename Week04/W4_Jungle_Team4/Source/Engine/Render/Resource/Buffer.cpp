#include "Buffer.h"

#pragma region __FMESHBUFFER__

void FMeshBuffer::Create(ID3D11Device* InDevice, const FMeshData& InMeshData)
{
	if (InMeshData.Vertices.empty())
	{
		VertexBuffer.Release();
		IndexBuffer.Release();
		return;
	}

	VertexBuffer.Create(InDevice, InMeshData.Vertices, static_cast<uint32>(sizeof(FVertex) * InMeshData.Vertices.size()), sizeof(FVertex));
	if (!InMeshData.Indices.empty())
	{
		IndexBuffer.Create(InDevice, InMeshData.Indices, static_cast<uint32>(sizeof(uint32) * InMeshData.Indices.size()));
	}
}

void FMeshBuffer::Release()
{
	VertexBuffer.Release();
	IndexBuffer.Release();
}

void FTextureMeshBuffer::Create(ID3D11Device* InDevice, const FTexturedMeshData& InMeshData)
{
	if (InMeshData.Vertices.empty())
	{
		VertexBuffer.Release();
		IndexBuffer.Release();
		return;
	}

	VertexBuffer.Create(InDevice, InMeshData.Vertices, static_cast<uint32>(sizeof(FTextureVertex) * InMeshData.Vertices.size()), sizeof(FTextureVertex));
	if (!InMeshData.Indices.empty())
	{
		IndexBuffer.Create(InDevice, InMeshData.Indices, static_cast<uint32>(sizeof(uint32) * InMeshData.Indices.size()));
	}
}

void FTextureMeshBuffer::Release()
{
	VertexBuffer.Release();
	IndexBuffer.Release();
}

#pragma endregion

#pragma region __FVERTEXBUFFER__

void FVertexBuffer::Create(ID3D11Device* InDevice, const TArray<FVertex>& InData, uint32 InByteWidth, uint32 InStride)
{
	if (InData.empty() || InByteWidth == 0)
	{
		Release();
		VertexCount = 0;
		Stride = InStride;
		return;
	}

	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = InByteWidth;
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = { InData.data() };

	HRESULT Hr = InDevice->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &Buffer);
	if (FAILED(Hr))
	{
		Release();
		VertexCount = 0;
		Stride = InStride;
		return;
	}

	VertexCount = static_cast<uint32>(InData.size());
	Stride = InStride;
}

void FVertexBuffer::Create(ID3D11Device* InDevice, const TArray<FTextureVertex>& InData, uint32 InByteWidth, uint32 InStride)
{
	if (InData.empty() || InByteWidth == 0)
	{
		Release();
		VertexCount = 0;
		Stride = InStride;
		return;
	}

	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = InByteWidth;
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = { InData.data() };

	HRESULT Hr = InDevice->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &Buffer);
	if (FAILED(Hr))
	{
		Release();
		VertexCount = 0;
		Stride = InStride;
		return;
	}

	VertexCount = static_cast<uint32>(InData.size());
	Stride = InStride;
}

void FVertexBuffer::Release()
{
	if (Buffer)
	{
		Buffer->Release();
		Buffer = nullptr;
	}
}

void FVertexBuffer::Update(ID3D11DeviceContext* InDeviceContext, const TArray<uint32>& InData, uint32 InByteWidth)
{
	(void)InDeviceContext;
	(void)InData;
	(void)InByteWidth;
}

ID3D11Buffer* FVertexBuffer::GetBuffer() const
{
	return Buffer;
}

#pragma endregion

#pragma region __FCONSTANTBUFFER__

void FConstantBuffer::Create(ID3D11Device* InDevice, uint32 InByteWidth)
{
	D3D11_BUFFER_DESC ConstantBufferDesc = {};

	ConstantBufferDesc.ByteWidth = (InByteWidth + 0xf) & 0xfffffff0;
	ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	InDevice->CreateBuffer(&ConstantBufferDesc, nullptr, &Buffer);
}

void FConstantBuffer::Release()
{
	if (Buffer)
	{
		Buffer->Release();
		Buffer = nullptr;
	}
}

void FConstantBuffer::Update(ID3D11DeviceContext* InDeviceContext, const void* InData, uint32 InByteWidth)
{
	if (Buffer)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;
		InDeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);

		std::memcpy(ConstantBufferMSR.pData, InData, InByteWidth);

		InDeviceContext->Unmap(Buffer, 0);
	}
}

ID3D11Buffer* FConstantBuffer::GetBuffer()
{
	return Buffer;
}

#pragma endregion

#pragma region __FINDEXBUFFER__

void FIndexBuffer::Create(ID3D11Device* InDevice, const TArray<uint32>& InData, uint32 InByteWidth)
{
	if (InData.empty() || InByteWidth == 0)
	{
		Release();
		IndexCount = 0;
		return;
	}

	D3D11_BUFFER_DESC IndexBufferDesc = {};
	IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	IndexBufferDesc.ByteWidth = InByteWidth;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IndexBufferSRD = { InData.data() };

	HRESULT Hr = InDevice->CreateBuffer(&IndexBufferDesc, &IndexBufferSRD, &Buffer);
	if (FAILED(Hr))
	{
		Release();
		IndexCount = 0;
		return;
	}

	IndexCount = static_cast<uint32>(InData.size());
}

void FIndexBuffer::Release()
{
	if (Buffer)
	{
		Buffer->Release();
		Buffer = nullptr;
	}
}

void FIndexBuffer::Update(ID3D11DeviceContext* InDeviceContext, const TArray<uint32>& InData, uint32 InByteWidth)
{
	(void)InDeviceContext;
	(void)InData;
	(void)InByteWidth;
}

ID3D11Buffer* FIndexBuffer::GetBuffer() const
{
	return Buffer;
}

#pragma endregion
