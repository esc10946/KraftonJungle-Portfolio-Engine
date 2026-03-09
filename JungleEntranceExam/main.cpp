#include <windows.h>

// D3D 사용에 필요한 라이브러리들을 링크
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D 사용에 필요한 헤더파일들을 포함
#include <d3d11.h>
#include <d3dcompiler.h>

// ImGui 관련 헤더
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

// 윈도우의 입력 이벤트를 ImGui에 전달하고, ImGui가 사용했는지 여부를 알려주는 함수
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/// <summary>
/// 윈도우에 이벤트가 발생할 경우 처리되는 콜백 함수
/// </summary>
/// <param name="hWnd"> 어떤 윈도우에 대한 메시지인지 </param>
/// <param name="message"> 어떤 이벤트인지 (종료, 키 입력, 마우스 입력 등) </param>
/// <param name="wParam"> 메시지에 딸려오는 추가 정보 </param>
/// <param name="lParam"> 메시지에 딸려오는 추가 정보 </param>
/// <returns></returns>
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGui의 이벤트 입력인지 판별
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

	switch (message)
	{
	case WM_DESTROY:
		// 윈도우가 닫힐 때
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam); // 윈도우 기본 처리 방식
	}

	return 0;
}

// 정점 구조체 선언
struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
};

// Structure for a 3D vector
struct FVector
{
    float x, y, z;
    FVector(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
};

// 구(Sphere) Vertices 정의가 포함된 헤더파일
#include "Sphere.h"

#pragma region 유틸 함수 모음
// 두 벡터의 내적 (Dot Product)
inline float Dot(const FVector& a, const FVector& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 벡터 곱
inline FVector Mul(const FVector& v, float s)
{
    return { v.x * s, v.y * s, v.z * s };
}

// 벡터 나눗셈
inline FVector Div(const FVector& v, float s)
{
    return { v.x / s, v.y / s, v.z / s };
}

// 두 벡터의 함
inline FVector Add(const FVector& a, const FVector& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

// 두 벡터의 차
inline FVector Sub(const FVector& a, const FVector& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

// 범위 내 랜덤 int값 생성 ( [min, max] )
static int RandomIntInRange(int min, int max)
{
    return min + rand() % (max - min + 1);
}

// 범위 내 랜덤 float값 생성 ( [min, max] )
static float RandomFloatInRange(float min, float max)
{
    return min + (max - min) * ((float)rand() / (float)RAND_MAX);
}
#pragma endregion

// 화면의 경계 위치를 나타내는 변수
const float leftBorder = -1.0f;
const float rightBorder = 1.0f;
const float topBorder = 1.0f;
const float bottomBorder = -1.0f;

/// <summary>
/// Direct3D 11 사용에 필요한 기본 생성, 소멸 기능을 담을 URenderer Class
/// </summary>
class URenderer
{
public:
    // Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
    ID3D11Device* Device = nullptr; // GPU와 통신하기 위한 Direct3D 장치
    ID3D11DeviceContext* DeviceContext = nullptr; // GPU 명령 실행을 담당하는 컨텍스트
    IDXGISwapChain* SwapChain = nullptr; // 프레임 버퍼를 교체하는 데 사용되는 스왑 체인

    // 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
    ID3D11Texture2D* FrameBuffer = nullptr; // 화면 출력용 텍스처
    ID3D11RenderTargetView* FrameBufferRTV = nullptr; // 텍스처를 렌더 타겟으로 사용하는 뷰
    ID3D11RasterizerState* RasterizerState = nullptr; // 래스터라이저 상태(컬링, 채우기 모드 등 정의)
    ID3D11Buffer* ConstantBuffer = nullptr; // 쉐이더에 데이터를 전달하기 위한 상수 버퍼

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
    D3D11_VIEWPORT ViewportInfo; // 렌더링 영역을 정의하는 뷰포트 정보

    ID3D11VertexShader* SimpleVertexShader;
    ID3D11PixelShader* SimplePixelShader;
    ID3D11InputLayout* SimpleInputLayout;
    unsigned int Stride;


public:
    // 렌더러 초기화 함수
    void Create(HWND hWindow)
    {
        // Direct3D 장치 및 스왑 체인 생성
        CreateDeviceAndSwapChain(hWindow);

        // 프레임 버퍼 생성
        CreateFrameBuffer();

        // 래스터라이저 상태 생성
        CreateRasterizerState();

        // 깊이 스텐실 버퍼 및 블렌드 상태는 이 코드에서는 다루지 않음
    }

    // Direct3D 장치 및 스왑 체인을 생성하는 함수
    void CreateDeviceAndSwapChain(HWND hWindow)
    {
        // 지원하는 Direct3D 기능 레벨을 정의
        D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

        // 스왑 체인 설정 구조체 초기화
        DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
        swapchaindesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
        swapchaindesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
        swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
        swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
        swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
        swapchaindesc.BufferCount = 2; // 더블 버퍼링
        swapchaindesc.OutputWindow = hWindow; // 렌더링할 창 핸들
        swapchaindesc.Windowed = TRUE; // 창 모드
        swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

        // Direct3D 장치와 스왑 체인을 생성
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
            featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
            &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

        // 생성된 스왑 체인의 정보 가져오기
        SwapChain->GetDesc(&swapchaindesc);

        // 뷰포트 정보 설정
        ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
    }

    // Direct3D 장치 및 스왑 체인을 해제하는 함수
    void ReleaseDeviceAndSwapChain()
    {
        if (DeviceContext)
        {
            DeviceContext->Flush(); // 남아있는 GPU 명령 실행
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

    // 프레임 버퍼를 생성하는 함수
    void CreateFrameBuffer()
    {
        // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
        SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

        // 렌더 타겟 뷰 생성
        D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
        framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
        framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

        Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
    }

    // 프레임 버퍼를 해제하는 함수
    void ReleaseFrameBuffer()
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

    // 래스터라이저 상태를 생성하는 함수
    void CreateRasterizerState()
    {
        D3D11_RASTERIZER_DESC rasterizerdesc = {};
        rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
        rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

        Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
    }

    // 래스터라이저 상태를 해제하는 함수
    void ReleaseRasterizerState()
    {
        if (RasterizerState)
        {
            RasterizerState->Release();
            RasterizerState = nullptr;
        }
    }

    // 렌더러에 사용된 모든 리소스를 해제하는 함수
    void Release()
    {
        RasterizerState->Release();

        // 렌더 타겟을 초기화
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

        ReleaseFrameBuffer();
        ReleaseDeviceAndSwapChain();
    }

    // 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
    void SwapBuffer()
    {
        SwapChain->Present(1, 0); // 1: VSync 활성화
    }

    // Shader를 생성하는 함수
    void CreateShader()
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

    // Shader 리소스를 해제하는 하는 함수
    void ReleaseShader()
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

    // 프레임 렌더링을 시작하기 위한 기본 상태 설정
    void Prepare()
    {
        DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

        DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        DeviceContext->RSSetViewports(1, &ViewportInfo);
        DeviceContext->RSSetState(RasterizerState);

        DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
        DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    }

    // 이번 그리기에서 사용할 쉐이더(VS/PS)와 정점 입력 레이아웃을 파이프라인에 바인딩
    void PrepareShader()
    {
        DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
        DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
        DeviceContext->IASetInputLayout(SimpleInputLayout);

        // 버텍스 쉐이더에 상수 버퍼를 설정
        if (ConstantBuffer)
        {
            DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
        }
    }

    // 정점 버퍼를 입력 어셈블러에 바인딩하고 지정한 정점 개수만큼 Draw
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
    {
        UINT offset = 0;
        DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);

        DeviceContext->Draw(numVertices, 0);
    }

    // Vertex Buffer 생성 함수
    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
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
    // Vertex Buffer를 Release 시키는 함수
    void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
    {
        vertexBuffer->Release();
    }

    // Constant Buffer(상수 버퍼) 관련 함수
    struct FConstants
    {
        FVector Offset;
        float Scale;
    };

    void CreateConstantBuffer()
    {
        D3D11_BUFFER_DESC constantbufferdesc = {};
        constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
        constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
        constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
    }

    void ReleaseConstantBuffer()
    {
        if (ConstantBuffer)
        {
            ConstantBuffer->Release();
            ConstantBuffer = nullptr;
        }
    }

    void UpdateConstant(FVector Offset, float Scale)
    {
        if (ConstantBuffer)
        {
            D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

            DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
            FConstants* constants = (FConstants*)constantbufferMSR.pData;
            {
                constants->Offset = Offset;
                constants->Scale = Scale;
            }
            DeviceContext->Unmap(ConstantBuffer, 0);
        }
    }
};

// 도형 기본 클래스
class UPrimitive
{
public:
    virtual ~UPrimitive() {}

    // 물리/이동 업데이트
    virtual void Update(float deltaTime) = 0;

    // 렌더링
    virtual void Render(URenderer& renderer) = 0;

    // 벽 충돌 적용
    virtual void ApplyWallCollision() = 0;

    // 중력 적용
    virtual void ApplyGravity(float deltaTime, const FVector& gravity) = 0;
};

// 공 클래스 (UPrimitive 상속 받음)
class UBall : public UPrimitive
{
public:
    FVector Location;           // 공의 위치
    FVector Velocity;           // 공의 속도
    float Radius;               // 공의 반지름
    float Mass;                 // 공의 질량 (반지름과 비례하다고 가정)
    static int TotalNumBalls;   // 총 공의 개수

    // 생성자 및 소멸자
public:
    UBall() : Location(0.0f, 0.0f, 0.0f), Velocity(0.0f, 0.0f, 0.0f), Radius(0.1f), Mass(0.1f)
    {
        ++TotalNumBalls;
        SetRadius(Radius);
    }

    virtual ~UBall() override
    {
        --TotalNumBalls;
    }


    // UPrimitive 인터페이스 구현
public:
    // 물리/이동 업데이트
    virtual void Update(float deltaTime)
    {
        // 속도에 기반하여 위치 적용
        Location.x += Velocity.x * deltaTime;
        Location.y += Velocity.y * deltaTime;
        Location.z += Velocity.z * deltaTime;

        // 벽 충돌 적용
        ApplyWallCollision();
    }

    // 렌더링 (상수 버퍼 업데이트)
    virtual void Render(URenderer& renderer)
    {
        renderer.UpdateConstant(Location, Radius);
    }

    // 벽 충돌 적용
    virtual void ApplyWallCollision()
    {
        // 벽과 충돌 여부를 체크하고 반사 시킴 (벽 끼임 방지 추가)
        if (Location.x < leftBorder + Radius)
        {
            Location.x = leftBorder + Radius;
            if (Velocity.x < 0.0f) Velocity.x *= -1.0f;
        }
        if (Location.x > rightBorder - Radius)
        {
            Location.x = rightBorder - Radius;
            if (Velocity.x > 0.0f) Velocity.x *= -1.0f;
        }
        if (Location.y > topBorder - Radius)
        {
            Location.y = topBorder - Radius;
            if (Velocity.y > 0.0f) Velocity.y *= -1.0f;
        }
        if (Location.y < bottomBorder + Radius)
        {
            Location.y = bottomBorder + Radius;
            if (Velocity.y < 0.0f) Velocity.y *= -1.0f;
        }
    }

    // 충격량 적용
    virtual void ApplyGravity(float deltaTime, const FVector& gravity)
    {
        Velocity = Add(Velocity, Mul(gravity, deltaTime));
    }

    
public:
    // 반지름 설정 (질량 자동 설정, 반지름에 비례)
    void SetRadius(float InRadius)
    {
        Radius = InRadius;

        Mass = Radius;
    }
};

// 공 생성 함수
static UPrimitive* CreateBall()
{
    // new 연산자를 사용해 공의 Instance를 생성
    UBall* Ball = new UBall();

    // 임의의 크기(Radius): 너무 큰 값을 방지하기 위해, 공의 크기를 화면 너비의 1/10로 제한
    float maxRadiusX = (rightBorder - leftBorder) * 0.05f;
    float maxRadiusY = (topBorder - bottomBorder) * 0.05f;
    float maxAllowedRadius = (maxRadiusX < maxRadiusY) ? maxRadiusX : maxRadiusY;
    float r = RandomFloatInRange(0.01f, maxAllowedRadius);
    Ball->SetRadius(r);
    
    // 임의의 위치(Location): 화면 경계 안쪽의 랜덤한 위치, 반지름을 마진값으로 함
    Ball->Location.x = RandomFloatInRange(leftBorder + Ball->Radius, rightBorder - Ball->Radius);
    Ball->Location.y = RandomFloatInRange(bottomBorder + Ball->Radius, topBorder - Ball->Radius);
    Ball->Location.z = 0.0f;

    // 임의의 속도(Velocity)
    Ball->Velocity.x = RandomFloatInRange(-0.2f, 0.2f);
    Ball->Velocity.y = RandomFloatInRange(-0.2f, 0.2f);
    Ball->Velocity.z = 0.0f;

    return Ball;
}

// Primitive List 크기를 증가시킬 때 호출
static void IncreasePrimitiveList(UPrimitive**& currPrimList, int& currNumBalls, int NewNumBalls)
{
    if (NewNumBalls <= currNumBalls) return;

    UPrimitive** NewList = new UPrimitive * [NewNumBalls];

    // 기존 포인터 복사하여 새로운 List 생성
    for (int i = 0; i < currNumBalls; ++i)
        NewList[i] = currPrimList[i];

    // 그 뒤에 새로운 공들을 생성
    for (int i = currNumBalls; i < NewNumBalls; ++i)
        NewList[i] = CreateBall();

    // 새로 만든 List로 교체
    delete[] currPrimList;
    currPrimList = NewList;

    currNumBalls = NewNumBalls;
}

// PrimitiveList 크기를 감소시킬 때 호출
static void DecreasePrimitiveList(UPrimitive**& currPrimList, int& currNumBalls, int NewNumBalls)
{
    if (NewNumBalls >= currNumBalls) return;

    while (currNumBalls > NewNumBalls)
    {
        // 임의의 공이 소멸 (소멸된 인덱스에는 맨 뒤의 값을 넣음)
        int delIndex = rand() % currNumBalls;

        delete currPrimList[delIndex];

        currPrimList[delIndex] = currPrimList[currNumBalls - 1];
        currPrimList[currNumBalls - 1] = nullptr;

        --currNumBalls;
    }

    // 배열 크기 줄이기
    UPrimitive** NewList = new UPrimitive * [currNumBalls];
    for (int i = 0; i < currNumBalls; ++i)
        NewList[i] = currPrimList[i];

    delete[] currPrimList;
    currPrimList = NewList;
}

// PrimitiveList를 재조정 하는 함수
static void AdjustPrimitiveListTo(UPrimitive**& currPrimList, int& currNumBalls, int NewNumBalls)
{
    if (NewNumBalls < 1) NewNumBalls = 1;

    if (NewNumBalls > currNumBalls)
        IncreasePrimitiveList(currPrimList, currNumBalls, NewNumBalls);
    else if (NewNumBalls < currNumBalls)
        DecreasePrimitiveList(currPrimList, currNumBalls, NewNumBalls);
}

// 두 공의 충돌 계산 함수
void ResolveBallCollision(UBall& ballA, UBall& ballB)
{
    // 충돌 검사
    FVector deltaVec = Sub(ballB.Location, ballA.Location);

    float radiusSum = ballA.Radius + ballB.Radius;
    float dist2 = Dot(deltaVec, deltaVec);

    if (dist2 >= radiusSum * radiusSum) return; // 비용 문제로, 먼저 거리 제곱으로 검사

    float dist = sqrtf(dist2);

    // 충돌 법선 단위 벡터 계산 (너무 작으면 임의 방향으로 설정)
    FVector n;
    float kEpsilon = 1e-6f; // 임의의 아주 아주 작은 값

    if (dist > kEpsilon)
        n = Div(deltaVec, dist);
    else
        n = FVector(1.0f, 0.0f, 0.0f);

    // 겹침 보정 (질량의 역수에 비례하여 침투한 만큼 밀려남)
    float mA = ballA.Radius;
    float mB = ballB.Radius;

    float inv_mA = (mA > kEpsilon) ? (1.0f / mA) : 0.0f;
    float inv_mB = (mB > kEpsilon) ? (1.0f / mB) : 0.0f;

    float penetration = radiusSum - dist;   // 겹침 정도
    float invSum = inv_mA + inv_mB;

    if (invSum > kEpsilon)
    {
        const float percent = 0.8f; // 밀어내는 정도에 대한 보정 값
        const float slop = 0.0005f; // 겹침 허용 오차

        float corrected = (penetration > slop) ? (penetration - slop) : 0.0f;   // 밀어낼 힘

        FVector correction = Mul(n, corrected * percent / invSum);    // 밀어낼 방향과 힘 벡터

        ballA.Location = Sub(ballA.Location, Mul(correction, inv_mA));
        ballB.Location = Add(ballB.Location, Mul(correction, inv_mB));
    }

    // 탄성 충돌
    FVector velocityA = ballA.Velocity;
    FVector velocityB = ballB.Velocity;

    FVector relativVel = Sub(velocityB, velocityA);   // 상대 속도
    float velAlongNormal = Dot(relativVel, n);

    if (velAlongNormal > 0.0f) return;  // 이미 서로 멀어지는 중 (충돌 스킵)

    float e = 1.0f; // 탄성계수

    float j = -(1.0f + e) * velAlongNormal; // 임펄스 j (튕겨지는 세기)
    j /= invSum;

    FVector impulse = Mul(n, j);

    velocityA = Sub(velocityA, Mul(impulse, inv_mA));
    velocityB = Add(velocityB, Mul(impulse, inv_mB));

    // 실제 velocity에 반영
    ballA.Velocity = velocityA;
    ballB.Velocity = velocityB;
}

// 공들의 충돌 처리 함수
void BallsCollision(UPrimitive** PrimitiveList, int NumPrimitives)
{
    // 공과 공 사이 충돌
    for (int i = 0; i < NumPrimitives; ++i)
    {
        UBall* ballA = static_cast<UBall*>(PrimitiveList[i]);

        for (int j = i + 1; j < NumPrimitives; ++j)
        {
            UBall* ballB = static_cast<UBall*>(PrimitiveList[j]);

            ResolveBallCollision(*ballA, *ballB);
        }
    }
}

// 공의 총 개수
int UBall::TotalNumBalls = 0;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// 윈도우 클래스 이름
	WCHAR WindowClass[] = L"JungleWindowClass";

	// 윈도우 타이틀바에 표시될 이름
	WCHAR Title[] = L"Game Tech Lab";

	// 이 클래스 이름(WindowClass)으로 생성된 모든 윈도우의 메시지는 WndProc()에서 처리된다.
	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// 위에서 만든 wndclass 라는 윈도우 클래스를 등록
	RegisterClassW(&wndclass);

	// 1024 x 1024 크기에 윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
		nullptr, nullptr, hInstance, nullptr);

	bool bIsExit = false;

    // random seed
    srand(GetTickCount());

    /* 생성하는 코드를 여기에 추가합니다. */

    // Renderer Class를 생성
    URenderer renderer;

    // D3D11을 사용한 Renderer 생성
    renderer.Create(hWnd);

    // Renderer 생성 직후, Shader 생성
    renderer.CreateShader();

    // Constant Buffer 생성
    renderer.CreateConstantBuffer();

    // ImGui를 사용하기 위한 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

    // Renderer와 Shader 생성 이후, vertexBuffer 생성
    UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

    ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));


    // 반드시 UBall이 아닌 UPrimitive로 선언하여야 하며 바꾸면 안됩니다.
    UPrimitive** PrimitiveList = nullptr;

    int NumPrimitives = 0;          // 현재 관리 중인 공 개수
    int TargetNumPrimitives = 1;    // UI로 조절하는 목표 공 개수

    // 중력 적용 여부
    bool bEnableGravity = true;

    // 중력 가속도 (기본 -Y 방향)
    const float gravityAccel = 9.8f;
    FVector gravityVec(0.0f, -gravityAccel, 0.0f);


    // FPS 제한
    const int targetFPS = 30;
    const double targetFrameTime = 1000.0 / targetFPS; // 한 프레임의 목표 시간 (밀리초 단위)

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;


	// Main Loop (Quit Message가 들어오기 전까지 아래 Loop를 무한히 실행하게 됨)
	while (bIsExit == false)
	{
        // 메인 루프 시작 시간 기록
        QueryPerformanceCounter(&startTime);

		MSG msg;

		// 처리할 메시지가 더 이상 없을때 까지 수행
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// 키 입력 메시지를 번역
			TranslateMessage(&msg);

			// 메시지를 적절한 윈도우 프로시저에 전달, 메시지가 위에서 등록한 WndProc 으로 전달됨
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)             // 종료 시
			{
				bIsExit = true;
				break;
			}
            else if (msg.message == WM_KEYDOWN)     // 키보드 눌렸을 때
            {
                // 입력된 방향키에 따라 중력의 방향을 변경
                if (msg.wParam == VK_LEFT)
                {
                    gravityVec = FVector(-gravityAccel, 0.0f, 0.0f);
                }
                if (msg.wParam == VK_RIGHT)
                {
                    gravityVec = FVector(gravityAccel, 0.0f, 0.0f);
                }
                if (msg.wParam == VK_UP)
                {
                    gravityVec = FVector(0.0f, gravityAccel, 0.0f);
                }
                if (msg.wParam == VK_DOWN)
                {
                    gravityVec = FVector(0.0f, -gravityAccel, 0.0f);
                }
            }
		}

		////////////////////////////////////////////
		// 매번 실행되는 코드를 여기에 추가합니다.

        // 준비 작업
        renderer.Prepare();
        renderer.PrepareShader();

        // 생성한 버텍스 버퍼를 넘겨 실질적인 렌더링 요청
        float dt = 1.0f / static_cast<float>(targetFPS);

        // 중력 적용
        if (bEnableGravity)
        {
            for (int i = 0; i < NumPrimitives; ++i)
            {
                PrimitiveList[i]->ApplyGravity(dt, gravityVec);
            }
        }

        // 업데이트
        for (int i = 0; i < NumPrimitives; ++i)
        {
            PrimitiveList[i]->Update(dt);
        }

        // 공과 공 사이의 충돌 계산
        BallsCollision(PrimitiveList, NumPrimitives);

        // 렌더링
        for (int i = 0; i < NumPrimitives; ++i)
        {
            PrimitiveList[i]->Render(renderer);

            // 실제 Draw
            renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
        }

        ImGui_ImplDX11_NewFrame();      // 렌더러(D3D11) 쪽에서 ImGui 프레임 준비
        ImGui_ImplWin32_NewFrame();     // 플랫폼(Win32) 쪽에서 ImGui 프레임 준비
        ImGui::NewFrame();

        /***** 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 ImGui::Render() 사이인 여기에 위치합니다. *****/
        ImGui::Begin("Jungle Property Window");
        
        ImGui::Text("Hello Jungle World!");

        ImGui::Checkbox("Gravity", &bEnableGravity);

        ImGui::InputInt("Number of Balls", &TargetNumPrimitives);

        if (TargetNumPrimitives < 1)
            TargetNumPrimitives = 1;

        // 목표 개수를 반영
        AdjustPrimitiveListTo(PrimitiveList, NumPrimitives, TargetNumPrimitives);

        ImGui::End();
        /*****                              (실제 ImGui UI를 코드로 그리는 구간)                         *****/

        ImGui::Render();    // 지금까지 쌓아둔 UI 명령들을 렌더링용 데이터(Draw Data)로 변환
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());    // 데이터(Draw Data)를 D3D11 Draw Call로 변환해서 실제로 그림

        // 다 그렸으면 버퍼 스왑
        renderer.SwapBuffer();

		////////////////////////////////////////////

        // 루프 실행 시간에 따라 FPS 제한
        do
        {
            Sleep(0);

            // 루프 종료 시간 기록
            QueryPerformanceCounter(&endTime);

            // 한 프레임이 소요된 시간 계산 (밀리초 단위로 변환)
            elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

        } while (elapsedTime < targetFrameTime);
	}

	/* 소멸하는 코드를 여기에 추가합니다. */

    // Primitive 관련
    for (int i = 0; i < NumPrimitives; ++i)
    {
        delete PrimitiveList[i];
        PrimitiveList[i] = nullptr;
    }
    delete[] PrimitiveList;
    PrimitiveList = nullptr;
    NumPrimitives = 0;

    // ImGui 관련
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // vertexBuffer 릴리즈
    renderer.ReleaseVertexBuffer(vertexBufferSphere);

    // Constant Buffer 릴리즈
    renderer.ReleaseConstantBuffer();

    // 렌더러 소멸 직전, 쉐이더 소멸
    renderer.ReleaseShader();

    // 렌더러 소멸
    renderer.Release();

	return 0;
}