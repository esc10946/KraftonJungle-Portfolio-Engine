#include "URenderer.h"

// ?뚮뜑??珥덇린???⑥닔
void URenderer::Create(HWND hWindow)
{
    // Direct3D ?μ튂 諛??ㅼ솑 泥댁씤 ?앹꽦
    CreateDeviceAndSwapChain(hWindow);

    // ?꾨젅??踰꾪띁 ?앹꽦
    CreateFrameBuffer();

    // ?섏뒪?곕씪?댁? ?곹깭 ?앹꽦
    CreateRasterizerState();

    // 源딆씠 ?ㅽ뀗??踰꾪띁 諛?釉붾젋???곹깭????肄붾뱶?먯꽌???ㅻ（吏 ?딆쓬
}

// Direct3D ?μ튂 諛??ㅼ솑 泥댁씤???앹꽦?섎뒗 ?⑥닔
void URenderer::CreateDeviceAndSwapChain(HWND hWindow)
{
    // 吏?먰븯??Direct3D 湲곕뒫 ?덈꺼???뺤쓽
    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // ?ㅼ솑 泥댁씤 ?ㅼ젙 援ъ“泥?珥덇린??
    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Width = 0; // 李??ш린??留욊쾶 ?먮룞?쇰줈 ?ㅼ젙
    swapchaindesc.BufferDesc.Height = 0; // 李??ш린??留욊쾶 ?먮룞?쇰줈 ?ㅼ젙
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ?됱긽 ?щ㎎
    swapchaindesc.SampleDesc.Count = 1; // 硫???섑뵆留?鍮꾪솢?깊솕
    swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ?뚮뜑 ?寃잛쑝濡??ъ슜
    swapchaindesc.BufferCount = 2; // ?붾툝 踰꾪띁留?
    swapchaindesc.OutputWindow = hWindow; // ?뚮뜑留곹븷 李??몃뱾
    swapchaindesc.Windowed = TRUE; // 李?紐⑤뱶
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ?ㅼ솑 諛⑹떇

    // Direct3D ?μ튂? ?ㅼ솑 泥댁씤???앹꽦
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
        featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
        &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

    // ?앹꽦???ㅼ솑 泥댁씤???뺣낫 媛?몄삤湲?
    SwapChain->GetDesc(&swapchaindesc);

    // 酉고룷???뺣낫 ?ㅼ젙
    ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
}

// Direct3D ?μ튂 諛??ㅼ솑 泥댁씤???댁젣?섎뒗 ?⑥닔
void URenderer::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // ?⑥븘?덈뒗 GPU 紐낅졊 ?ㅽ뻾
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

// ?꾨젅??踰꾪띁瑜??앹꽦?섎뒗 ?⑥닔
void URenderer::CreateFrameBuffer()
{
    // ?ㅼ솑 泥댁씤?쇰줈遺??諛?踰꾪띁 ?띿뒪泥?媛?몄삤湲?
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

    // ?뚮뜑 ?寃?酉??앹꽦
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // ?됱긽 ?щ㎎
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D ?띿뒪泥?

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
}

// ?꾨젅??踰꾪띁瑜??댁젣?섎뒗 ?⑥닔
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

// ?섏뒪?곕씪?댁? ?곹깭瑜??앹꽦?섎뒗 ?⑥닔
void URenderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 梨꾩슦湲?紐⑤뱶
    rasterizerdesc.CullMode = D3D11_CULL_BACK; // 諛??섏씠??而щ쭅

    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
}

// ?섏뒪?곕씪?댁? ?곹깭瑜??댁젣?섎뒗 ?⑥닔
void URenderer::ReleaseRasterizerState()
{
    if (RasterizerState)
    {
        RasterizerState->Release();
        RasterizerState = nullptr;
    }
}

// ?뚮뜑?ъ뿉 ?ъ슜??紐⑤뱺 由ъ냼?ㅻ? ?댁젣?섎뒗 ?⑥닔
void URenderer::Release()
{
    RasterizerState->Release();

    // ?뚮뜑 ?寃잛쓣 珥덇린??
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseFrameBuffer();
    ReleaseDeviceAndSwapChain();
}

// ?ㅼ솑 泥댁씤??諛?踰꾪띁? ?꾨줎??踰꾪띁瑜?援먯껜?섏뿬 ?붾㈃??異쒕젰
void URenderer::SwapBuffer()
{
    SwapChain->Present(1, 0); // 1: VSync ?쒖꽦??
}

// Shader瑜??앹꽦?섎뒗 ?⑥닔
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

// Shader 由ъ냼?ㅻ? ?댁젣?섎뒗 ?섎뒗 ?⑥닔
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

// ?꾨젅???뚮뜑留곸쓣 ?쒖옉?섍린 ?꾪븳 湲곕낯 ?곹깭 ?ㅼ젙
void URenderer::Prepare()
{
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DeviceContext->RSSetViewports(1, &ViewportInfo);
    DeviceContext->RSSetState(RasterizerState);

    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

// ?대쾲 洹몃━湲곗뿉???ъ슜???먯씠??VS/PS)? ?뺤젏 ?낅젰 ?덉씠?꾩썐???뚯씠?꾨씪?몄뿉 諛붿씤??
void URenderer::PrepareShader()
{
    DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
    DeviceContext->IASetInputLayout(SimpleInputLayout);

    // 踰꾪뀓???먯씠?붿뿉 ?곸닔 踰꾪띁瑜??ㅼ젙
    if (ConstantBuffer)
    {
        DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
        DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
    }
}

// ?뺤젏 踰꾪띁瑜??낅젰 ?댁뀍釉붾윭??諛붿씤?⑺븯怨?吏?뺥븳 ?뺤젏 媛쒖닔留뚰겮 Draw
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

// Vertex Buffer ?앹꽦 ?⑥닔
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
// Vertex Buffer瑜?Release ?쒗궎???⑥닔
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

        {-1.0f + bx,  1.0f - by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // 醫뚯긽
        { 1.0f - bx, -1.0f + by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // ?고븯
        {-1.0f + bx, -1.0f + by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // 醫뚰븯
        {-1.0f + bx,  1.0f - by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // 醫뚯긽
        { 1.0f - bx,  1.0f - by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // ?곗긽
        { 1.0f - bx, -1.0f + by, 0.0f, 1.0f, 1.0f, 1.0f, 1}, // ?고븯

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
            constants->BlockColor = FColor(1, 1, 1, 1);
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
