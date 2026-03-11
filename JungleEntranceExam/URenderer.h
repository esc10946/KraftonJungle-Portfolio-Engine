#pragma once

// D3D ?ъ슜???꾩슂???ㅻ뜑?뚯씪?ㅼ쓣 ?ы븿
#include <d3d11.h>
#include <d3dcompiler.h>

#include "FVertexSimple.h"
#include "FVector.h"
#include "FColor.h"

// Constant Buffer(?곸닔 踰꾪띁) 愿???⑥닔
//struct FConstants
//{
//    FVector Offset;
//    float Scale;
//};

struct FConstants
{
    FVector Offset;
    float WipeProgress;
    FVector Scale;
    float Pad2;
    FColor BlockColor;
};

class URenderer
{
public:
    // Direct3D 11 ?μ튂(Device)? ?μ튂 而⑦뀓?ㅽ듃(Device Context) 諛??ㅼ솑 泥댁씤(Swap Chain)??愿由ы븯湲??꾪븳 ?ъ씤?곕뱾
    ID3D11Device* Device = nullptr; // GPU? ?듭떊?섍린 ?꾪븳 Direct3D ?μ튂
    ID3D11DeviceContext* DeviceContext = nullptr; // GPU 紐낅졊 ?ㅽ뻾???대떦?섎뒗 而⑦뀓?ㅽ듃
    IDXGISwapChain* SwapChain = nullptr; // ?꾨젅??踰꾪띁瑜?援먯껜?섎뒗 ???ъ슜?섎뒗 ?ㅼ솑 泥댁씤

    // ?뚮뜑留곸뿉 ?꾩슂??由ъ냼??諛??곹깭瑜?愿由ы븯湲??꾪븳 蹂?섎뱾
    ID3D11Texture2D* FrameBuffer = nullptr; // ?붾㈃ 異쒕젰???띿뒪泥?
    ID3D11RenderTargetView* FrameBufferRTV = nullptr; // ?띿뒪泥섎? ?뚮뜑 ?寃잛쑝濡??ъ슜?섎뒗 酉?
    ID3D11RasterizerState* RasterizerState = nullptr; // ?섏뒪?곕씪?댁? ?곹깭(而щ쭅, 梨꾩슦湲?紐⑤뱶 ???뺤쓽)
    ID3D11Buffer* ConstantBuffer = nullptr; // ?먯씠?붿뿉 ?곗씠?곕? ?꾨떖?섍린 ?꾪븳 ?곸닔 踰꾪띁

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // ?붾㈃??珥덇린??clear)?????ъ슜???됱긽 (RGBA)
    D3D11_VIEWPORT ViewportInfo; // ?뚮뜑留??곸뿭???뺤쓽?섎뒗 酉고룷???뺣낫

    ID3D11VertexShader* SimpleVertexShader=nullptr;
    ID3D11PixelShader* SimplePixelShader = nullptr;
    ID3D11InputLayout* SimpleInputLayout = nullptr;
    unsigned int Stride;

    UINT NumVerticesSphere=0;
    UINT NumVerticesBar=12;
    UINT NumVerticesRect = 12;

    ID3D11Buffer* vertexBufferSphere = nullptr;
    ID3D11Buffer* vertexBufferBar = nullptr;
    ID3D11Buffer* vertexBufferRect = nullptr;
    ID3D11Buffer* vertexBufferTriangle = nullptr;

public:
    void Create(HWND hWindow);
    void CreateDeviceAndSwapChain(HWND hWindow);
    void ReleaseDeviceAndSwapChain();
    void CreateFrameBuffer();
    void ReleaseFrameBuffer();
    void CreateRasterizerState();
    void ReleaseRasterizerState();
    void Release();
    void SwapBuffer();
    void CreateShader();
    void ReleaseShader();
    void Prepare();
    void PrepareShader();
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
    void RenderSphere();
    void RenderRectangle();
    void RenderTriangle();

    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
    void ReleaseVertexBuffer();
    void ReleaseVertexBuffer(ID3D11Buffer* pbuffer);
    void CreateConstantBuffer();
    void ReleaseConstantBuffer();
    void UpdateConstant(FVector Offset, FVector Scale);

    void UpdateConstant(FVector Offset, FVector Scale, FColor Color, float WipeProgress=-3.0f);

    void CreateRectBuffer();
    void ReleaseRectBuffer();


    //void RenderRect(float cx, float cy, float hw, float hh, float progress, FColor Color);
};
