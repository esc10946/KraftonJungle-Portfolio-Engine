#pragma once

// D3D ��뿡 �ʿ��� ������ϵ��� ����
#include <d3d11.h>
#include <d3dcompiler.h>

#include "FVertexSimple.h"
#include "FVector.h"
#include "FColor.h"

// Constant Buffer(��� ����) ���� �Լ�
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
    // Direct3D 11 ��ġ(Device)�� ��ġ ���ؽ�Ʈ(Device Context) �� ���� ü��(Swap Chain)�� �����ϱ� ���� �����͵�
    ID3D11Device* Device = nullptr; // GPU�� ����ϱ� ���� Direct3D ��ġ
    ID3D11DeviceContext* DeviceContext = nullptr; // GPU ��� ������ ����ϴ� ���ؽ�Ʈ
    IDXGISwapChain* SwapChain = nullptr; // ������ ���۸� ��ü�ϴ� �� ���Ǵ� ���� ü��

    // �������� �ʿ��� ���ҽ� �� ���¸� �����ϱ� ���� ������
    ID3D11Texture2D* FrameBuffer = nullptr; // ȭ�� ��¿� �ؽ�ó
    ID3D11RenderTargetView* FrameBufferRTV = nullptr; // �ؽ�ó�� ���� Ÿ������ ����ϴ� ��
    ID3D11RasterizerState* RasterizerState = nullptr; // �����Ͷ����� ����(�ø�, ä��� ��� �� ����)
    ID3D11Buffer* ConstantBuffer = nullptr; // ���̴��� �����͸� �����ϱ� ���� ��� ����

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // ȭ���� �ʱ�ȭ(clear)�� �� ����� ���� (RGBA)
    D3D11_VIEWPORT ViewportInfo; // ������ ������ �����ϴ� ����Ʈ ����

    ID3D11VertexShader* SimpleVertexShader=nullptr;
    ID3D11PixelShader* SimplePixelShader = nullptr;
    ID3D11InputLayout* SimpleInputLayout = nullptr;
    unsigned int Stride;

    UINT NumVerticesBullet;
    UINT NumVerticesSphere=0;
    UINT NumVerticesBar=12;
    UINT NumVerticesRect = 12;

    ID3D11Buffer* vertexBufferSphere = nullptr;
    ID3D11Buffer* vertexBufferBar = nullptr;
    ID3D11Buffer* vertexBufferRect = nullptr;
    ID3D11Buffer* vertexBufferTriangle = nullptr;
    ID3D11Buffer* vertexBufferBullet = nullptr;

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
    void RenderBullet();

    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
    void ReleaseVertexBuffer();
    void ReleaseVertexBuffer(ID3D11Buffer* pbuffer);
    void CreateConstantBuffer();
    void ReleaseConstantBuffer();
    void UpdateConstant(FVector Offset, FVector Scale);
    void UpdateConstant(FVector Offset, FVector Scale, float alpha);

    void UpdateConstant(FVector Offset, FVector Scale, FColor Color, float WipeProgress=-3.0f);

    void CreateRectBuffer();
    void ReleaseRectBuffer();

  


    //void RenderRect(float cx, float cy, float hw, float hh, float progress, FColor Color);
};
