#include "URenderer.h"

// ������ �ʱ�ȭ �Լ�
void URenderer::Create(HWND hWindow)
{
    // Direct3D ��ġ �� ���� ü�� ����
    CreateDeviceAndSwapChain(hWindow);

    // ������ ���� ����
    CreateFrameBuffer();

    // �����Ͷ����� ���� ����
    CreateRasterizerState();

    // ���� ���ٽ� ���� �� ����� ���´� �� �ڵ忡���� �ٷ��� ����
}

// Direct3D ��ġ �� ���� ü���� �����ϴ� �Լ�
void URenderer::CreateDeviceAndSwapChain(HWND hWindow)
{
    // �����ϴ� Direct3D ��� ������ ����
    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // ���� ü�� ���� ����ü �ʱ�ȭ
    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Width = 0; // â ũ�⿡ �°� �ڵ����� ����
    swapchaindesc.BufferDesc.Height = 0; // â ũ�⿡ �°� �ڵ����� ����
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ���� ����
    swapchaindesc.SampleDesc.Count = 1; // ��Ƽ ���ø� ��Ȱ��ȭ
    swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ���� Ÿ������ ���
    swapchaindesc.BufferCount = 2; // ���� ���۸�
    swapchaindesc.OutputWindow = hWindow; // �������� â �ڵ�
    swapchaindesc.Windowed = TRUE; // â ���
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ���� ���

    // Direct3D ��ġ�� ���� ü���� ����
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
        featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
        &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

    // ������ ���� ü���� ���� ��������
    SwapChain->GetDesc(&swapchaindesc);

    // ����Ʈ ���� ����
    ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
}

// Direct3D ��ġ �� ���� ü���� �����ϴ� �Լ�
void URenderer::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // �����ִ� GPU ��� ����
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }
}

// ������ ���۸� �����ϴ� �Լ�
void URenderer::CreateFrameBuffer()
{
    // ���� ü�����κ��� �� ���� �ؽ�ó ��������
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

    // ���� Ÿ�� �� ����
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // ���� ����
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D �ؽ�ó

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
}

// ������ ���۸� �����ϴ� �Լ�
void URenderer::ReleaseFrameBuffer()
{
    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }

    if (FrameBufferRTV)
    {
        FrameBufferRTV->Release();
        FrameBufferRTV = nullptr;
    }
}

// �����Ͷ����� ���¸� �����ϴ� �Լ�
void URenderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID; // ä��� ���
    rasterizerdesc.CullMode = D3D11_CULL_BACK; // �� ���̽� �ø�

    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
}

// �����Ͷ����� ���¸� �����ϴ� �Լ�
void URenderer::ReleaseRasterizerState()
{
    if (RasterizerState)
    {
        RasterizerState->Release();
        RasterizerState = nullptr;
    }
}

// �������� ���� ��� ���ҽ��� �����ϴ� �Լ�
void URenderer::Release()
{
    RasterizerState->Release();

    // ���� Ÿ���� �ʱ�ȭ
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseFrameBuffer();
    ReleaseDeviceAndSwapChain();
}

// ���� ü���� �� ���ۿ� ����Ʈ ���۸� ��ü�Ͽ� ȭ�鿡 ���
void URenderer::SwapBuffer()
{
    SwapChain->Present(1, 0); // 1: VSync Ȱ��ȭ
}

// Shader�� �����ϴ� �Լ�
void URenderer::CreateShader()
{
    ID3DBlob* vertexshaderCSO;
    ID3DBlob* pixelshaderCSO;

    D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);

    Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

    D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);

    Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);

    Stride = sizeof(FVertexSimple);

    vertexshaderCSO->Release();
    pixelshaderCSO->Release();
}

// Shader ���ҽ��� �����ϴ� �ϴ� �Լ�
void URenderer::ReleaseShader()
{
    if (SimpleInputLayout)
    {
        SimpleInputLayout->Release();
        SimpleInputLayout = nullptr;
    }

    if (SimplePixelShader)
    {
        SimplePixelShader->Release();
        SimplePixelShader = nullptr;
    }

    if (SimpleVertexShader)
    {
        SimpleVertexShader->Release();
        SimpleVertexShader = nullptr;
    }
}

// ������ �������� �����ϱ� ���� �⺻ ���� ����
void URenderer::Prepare()
{
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));

    blendDesc.RenderTarget[0].BlendEnable = TRUE; // 블렌딩 활성화
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DeviceContext->RSSetViewports(1, &ViewportInfo);
    DeviceContext->RSSetState(RasterizerState);

    ID3D11BlendState* pBlendState;
    Device->CreateBlendState(&blendDesc, &pBlendState);
    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
    DeviceContext->OMSetBlendState(pBlendState, nullptr, 0xffffffff);
}

// �̹� �׸��⿡�� ����� ���̴�(VS/PS)�� ���� �Է� ���̾ƿ��� ���������ο� ���ε�
void URenderer::PrepareShader()
{
    DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
    DeviceContext->IASetInputLayout(SimpleInputLayout);

    // ���ؽ� ���̴��� ��� ���۸� ����
    if (ConstantBuffer)
    {
        DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
        DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
    }
}

// ���� ���۸� �Է� �������� ���ε��ϰ� ������ ���� ������ŭ Draw
void URenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);

    DeviceContext->Draw(numVertices, 0);
}

void URenderer::RenderSphere()
{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBufferSphere, &Stride, &offset);
    DeviceContext->Draw(NumVerticesSphere, 0);
}

void URenderer::RenderRectangle()
{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBufferRect, &Stride, &offset);
    DeviceContext->Draw(NumVerticesRect, 0);
}

void URenderer::RenderTriangle()
{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBufferTriangle, &Stride, &offset);
    DeviceContext->Draw(3, 0);
}

void URenderer::RenderBullet()
{
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBufferBullet, &Stride, &offset);
    DeviceContext->Draw(NumVerticesBullet, 0);
}

// Vertex Buffer ���� �Լ�
ID3D11Buffer* URenderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
{
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

    ID3D11Buffer* vertexBuffer;

    Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

    return vertexBuffer;
}
// Vertex Buffer�� Release ��Ű�� �Լ�
void URenderer::ReleaseVertexBuffer()
{
    if (vertexBufferRect)
    {
        vertexBufferRect->Release();
        vertexBufferRect = nullptr; 
    }

    if (vertexBufferSphere)
    {
        vertexBufferSphere->Release();
        vertexBufferSphere = nullptr;
    }

    if (vertexBufferTriangle)
    {
        vertexBufferTriangle->Release();
        vertexBufferTriangle = nullptr;
    }
}

void URenderer::ReleaseVertexBuffer(ID3D11Buffer* pbuffer)
{
    if(pbuffer)
        pbuffer->Release();
}

void URenderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
}

void URenderer::ReleaseConstantBuffer()
{
    if (ConstantBuffer)
    {
        ConstantBuffer->Release();
        ConstantBuffer = nullptr;
    }
}

//void URenderer::UpdateConstant(FVector Offset, float Scale)
//{
//    if (ConstantBuffer)
//    {
//        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
//
//        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
//        FConstants* constants = (FConstants*)constantbufferMSR.pData;
//        {
//            constants->Offset = Offset;
//            constants->Scale = Scale;
//        }
//        DeviceContext->Unmap(ConstantBuffer, 0);
//    }
//}

//void URenderer::CreateRectBuffer()
//{
//    const FColor fill = { 1,1,1,1 };
//    const FColor border = { 0.3f,0.3f,0.3f,1 };
//    const float bx = 0.10f, by = 0.12f;
//    FVertexSimple verts[12] =
//    {
//
//        {-1,  1, 0, border.r, border.g, border.b, 1},
//        { 1, -1, 0, border.r, border.g, border.b, 1},
//        {-1, -1, 0, border.r, border.g, border.b, 1},
//        {-1,  1, 0, border.r, border.g, border.b, 1},
//        { 1,  1, 0, border.r, border.g, border.b, 1},
//        { 1, -1, 0, border.r, border.g, border.b, 1},
//
//        {-1 + bx,  1 - by, 0, fill.r, fill.g, fill.b, 1},
//        { 1 - bx, -1 + by, 0, fill.r, fill.g, fill.b, 1},
//        {-1 + bx, -1 + by, 0, fill.r, fill.g, fill.b, 1},
//        {-1 + bx,  1 - by, 0, fill.r, fill.g, fill.b, 1},
//        { 1 - bx,  1 - by, 0, fill.r, fill.g, fill.b, 1},
//        { 1 - bx, -1 + by, 0, fill.r, fill.g, fill.b, 1},
//    };
//    RectVB = CreateVertexBuffer(verts, sizeof(verts));
//}
//void ReleaseRectBuffer()
//{
//    if (RectVB) { RectVB->Release(); RectVB = nullptr; }
//}

void URenderer::CreateRectBuffer()
{

	const float bx = 0.05f, by = 0.12f;
    FVertexSimple verts[12] =
    {
        {-1.0f,  1.0f, 0.0f,0.3f,0.3f,0.3f,0 },
        { 1.0f, -1.0f, 0.0f,0.3f,0.3f,0.3f,0 },
        {-1.0f, -1.0f, 0.0f,0.3f,0.3f,0.3f,0 },
        {-1.0f,  1.0f, 0.0f,0.3f,0.3f,0.3f,0 },
        { 1.0f,  1.0f, 0.0f,0.3f,0.3f,0.3f,0 },
        { 1.0f, -1.0f, 0.0f,0.3f,0.3f,0.3f,0 },

        {-1.0f + bx,  1.0f - by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // �»�
        { 1.0f - bx, -1.0f + by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // ����
        {-1.0f + bx, -1.0f + by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // ����
        {-1.0f + bx,  1.0f - by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // �»�
        { 1.0f - bx,  1.0f - by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // ���
        { 1.0f - bx, -1.0f + by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // ����

    };
	vertexBufferRect = CreateVertexBuffer(verts, sizeof(verts));
    NumVerticesRect = 12;
}
void URenderer::ReleaseRectBuffer()
{
    if (vertexBufferRect)
    {
		vertexBufferRect->Release();
		vertexBufferRect = nullptr;
    }
}
void URenderer::UpdateConstant(FVector Offset, FVector Scale)
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            constants->Offset = Offset;
            constants->WipeProgress = -3.0f;
            constants->Scale = Scale;
            constants->FlashTimer = 0;

            constants->BlockColor = FColor(1, 1, 1, 1);
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}

void URenderer::UpdateConstant(FVector Offset, FVector Scale, FColor Color, float WipeProgress, float FlashTimer)
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        FConstants* constants = (FConstants*)constantbufferMSR.pData; {
            constants->Offset = Offset;
            constants->WipeProgress = WipeProgress;
            constants->Scale = Scale;
            constants->FlashTimer = FlashTimer;
            constants->BlockColor = Color;
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}


void URenderer::UpdateConstant(FVector Offset, FVector Scale,FColor Color, float WipeProgress)
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            constants->Offset = Offset;
            constants->WipeProgress = WipeProgress;
            constants->Scale = Scale;
            constants->FlashTimer = 0;

            constants->BlockColor = Color;
         
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}

//void URenderer::RenderRect(float cx, float cy, float hw, float hh, float Progress,FColor Color)
//{
//    const FColor BgColor = Color * 0.5f;
//    const float L = cx - hw;
//    const float R = cx + hw;
//    const float T = cy + hh;
//    const float B = cy - hh;
//
//    const float brdX = hw * 0.10f;
//    const float brdY = hh * 0.12f;
//
//    const float IL = L + brdX;
//    const float IR = R - brdX;
//    const float IT = T - brdY;
//    const float IB = B + brdY;
//
//    FVertexSimple verts[12] =
//    {
//
//        { L,  T,  0.0f,  BgColor.r, BgColor.g, BgColor.b, 1.0f },
//        { R,  B,  0.0f,  BgColor.r, BgColor.g, BgColor.b, 1.0f },
//        { L,  B,  0.0f,  BgColor.r, BgColor.g, BgColor.b, 1.0f },
//
//        { L,  T,  0.0f,  BgColor.r, BgColor.g, BgColor.b, 1.0f },
//        { R,  T,  0.0f,  BgColor.r, BgColor.g, BgColor.b, 1.0f },
//        { R,  B,  0.0f,  BgColor.r, BgColor.g, BgColor.b, 1.0f },
//
//
//        { IL, IT, 0.0f,  Color.r, Color.g, Color.b, 1.0f },
//        { IR, IB, 0.0f,  Color.r, Color.g, Color.b, 1.0f },
//        { IL, IB, 0.0f,  Color.r, Color.g, Color.b, 1.0f },
//
//        { IL, IT, 0.0f,  Color.r, Color.g, Color.b, 1.0f },
//        { IR, IT, 0.0f,  Color.r, Color.g, Color.b, 1.0f },
//        { IR, IB, 0.0f,  Color.r, Color.g, Color.b, 1.0f },
//    };
//    ID3D11Buffer* vb = CreateVertexBuffer(verts, sizeof(verts));
//
//    UpdateConstant(FVector(0.0f, 0.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f), Progress);
//    RenderPrimitive(vb, 12);
//
//    ReleaseVertexBuffer(vb);
//}