#include <windows.h>

// D3D 사용에 필요한 라이브러리들을 링크
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D 사용에 필요한 헤더파일들을 포함
#include "URenderer.h"
#include "UDiagram.h"
#include "UBar.h"
#include "Bar.h"
#include "UBall.h"
#include "Sphere.h"
#include "UBlock.h"
#include "Util.h"

// ImGui 관련 헤더
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Shape.h"

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

// 공 생성 함수
static UBall* CreateBall()
{
    // new 연산자를 사용해 공의 Instance를 생성
    UBall* Ball = new UBall();

    // 임의의 크기(Radius): 너무 큰 값을 방지하기 위해, 공의 크기를 화면 너비의 1/10로 제한
    float maxRadiusX = (rightBorder - leftBorder) * 0.05f;
    float maxRadiusY = (topBorder - bottomBorder) * 0.05f;
    float maxAllowedRadius = (maxRadiusX < maxRadiusY) ? maxRadiusX : maxRadiusY;
    float r = GetRandomFloat(0.01f, maxAllowedRadius);
    Ball->SetRadius(r);
    
    // 임의의 위치(Location): 화면 경계 안쪽의 랜덤한 위치, 반지름을 마진값으로 함
    Ball->Location.x = GetRandomFloat(leftBorder + Ball->Radius, rightBorder - Ball->Radius);
    Ball->Location.y = GetRandomFloat(bottomBorder + Ball->Radius, topBorder - Ball->Radius);
    Ball->Location.z = 0.0f;

    // 임의의 속도(Velocity)
    Ball->Velocity.x = GetRandomFloat(1.5f, 2.0f);
    Ball->Velocity.y = GetRandomFloat(1.5f, 2.0f);
    Ball->Velocity.z = 0.0f;

    return Ball;
}

static void InitBall(UBall& input)
{
    // 임의의 크기(Radius): 너무 큰 값을 방지하기 위해, 공의 크기를 화면 너비의 1/10로 제한
    float maxRadiusX = (rightBorder - leftBorder) * 0.05f;
    float maxRadiusY = (topBorder - bottomBorder) * 0.05f;
    float maxAllowedRadius = (maxRadiusX < maxRadiusY) ? maxRadiusX : maxRadiusY;
    float r = 0.05f; //GetRandomFloat(0.1f, 0.2f);
    input.SetRadius(r);

    // 임의의 위치(Location): 화면 경계 안쪽의 랜덤한 위치, 반지름을 마진값으로 함
    input.Location.x = GetRandomFloat(leftBorder + input.Radius, rightBorder - input.Radius);
    input.Location.y = GetRandomFloat(bottomBorder + input.Radius, topBorder - input.Radius);
    input.Location.z = 0.0f;

    // 임의의 속도(Velocity)
    input.Velocity.x = 0.0f; //GetRandomFloat(1.5f, 2.0f);
    input.Velocity.y = 1.0f; //GetRandomFloat(1.5f, 2.0f);
    input.Velocity.z = 0.0f;
}

//// Primitive List 크기를 증가시킬 때 호출
//static void IncreasePrimitiveList(UPrimitive**& currPrimList, int& currNumBalls, int NewNumBalls)
//{
//    if (NewNumBalls <= currNumBalls) return;
//
//    UPrimitive** NewList = new UPrimitive * [NewNumBalls];
//
//    // 기존 포인터 복사하여 새로운 List 생성
//    for (int i = 0; i < currNumBalls; ++i)
//        NewList[i] = currPrimList[i];
//
//    // 그 뒤에 새로운 공들을 생성
//    for (int i = currNumBalls; i < NewNumBalls; ++i)
//        NewList[i] = CreateBall();
//
//    // 새로 만든 List로 교체
//    delete[] currPrimList;
//    currPrimList = NewList;
//
//    currNumBalls = NewNumBalls;
//}
//
//// PrimitiveList 크기를 감소시킬 때 호출
//static void DecreasePrimitiveList(UPrimitive**& currPrimList, int& currNumBalls, int NewNumBalls)
//{
//    if (NewNumBalls >= currNumBalls) return;
//
//    while (currNumBalls > NewNumBalls)
//    {
//        // 임의의 공이 소멸 (소멸된 인덱스에는 맨 뒤의 값을 넣음)
//        int delIndex = rand() % currNumBalls;
//
//        delete currPrimList[delIndex];
//
//        currPrimList[delIndex] = currPrimList[currNumBalls - 1];
//        currPrimList[currNumBalls - 1] = nullptr;
//
//        --currNumBalls;
//    }
//
//    // 배열 크기 줄이기
//    UPrimitive** NewList = new UPrimitive * [currNumBalls];
//    for (int i = 0; i < currNumBalls; ++i)
//        NewList[i] = currPrimList[i];
//
//    delete[] currPrimList;
//    currPrimList = NewList;
//}
//
//// PrimitiveList를 재조정 하는 함수
//static void AdjustPrimitiveListTo(UPrimitive**& currPrimList, int& currNumBalls, int NewNumBalls)
//{
//    if (NewNumBalls < 1) NewNumBalls = 1;
//
//    if (NewNumBalls > currNumBalls)
//        IncreasePrimitiveList(currPrimList, currNumBalls, NewNumBalls);
//    else if (NewNumBalls < currNumBalls)
//        DecreasePrimitiveList(currPrimList, currNumBalls, NewNumBalls);
//}
//
//// 두 공의 충돌 계산 함수
//void ResolveBallCollision(UBall& ballA, UBall& ballB)
//{
//    // 충돌 검사
//    FVector deltaVec = Sub(ballB.Location, ballA.Location);
//
//    float radiusSum = ballA.Radius + ballB.Radius;
//    float dist2 = Dot(deltaVec, deltaVec);
//
//    if (dist2 >= radiusSum * radiusSum) return; // 비용 문제로, 먼저 거리 제곱으로 검사
//
//    float dist = sqrtf(dist2);
//
//    // 충돌 법선 단위 벡터 계산 (너무 작으면 임의 방향으로 설정)
//    FVector n;
//    float kEpsilon = 1e-6f; // 임의의 아주 아주 작은 값
//
//    if (dist > kEpsilon)
//        n = Div(deltaVec, dist);
//    else
//        n = FVector(1.0f, 0.0f, 0.0f);
//
//    // 겹침 보정 (질량의 역수에 비례하여 침투한 만큼 밀려남)
//    float mA = ballA.Radius;
//    float mB = ballB.Radius;
//
//    float inv_mA = (mA > kEpsilon) ? (1.0f / mA) : 0.0f;
//    float inv_mB = (mB > kEpsilon) ? (1.0f / mB) : 0.0f;
//
//    float penetration = radiusSum - dist;   // 겹침 정도
//    float invSum = inv_mA + inv_mB;
//
//    if (invSum > kEpsilon)
//    {
//        const float percent = 0.8f; // 밀어내는 정도에 대한 보정 값
//        const float slop = 0.0005f; // 겹침 허용 오차
//
//        float corrected = (penetration > slop) ? (penetration - slop) : 0.0f;   // 밀어낼 힘
//
//        FVector correction = Mul(n, corrected * percent / invSum);    // 밀어낼 방향과 힘 벡터
//
//        ballA.Location = Sub(ballA.Location, Mul(correction, inv_mA));
//        ballB.Location = Add(ballB.Location, Mul(correction, inv_mB));
//    }
//
//    // 탄성 충돌
//    FVector velocityA = ballA.Velocity;
//    FVector velocityB = ballB.Velocity;
//
//    FVector relativVel = Sub(velocityB, velocityA);   // 상대 속도
//    float velAlongNormal = Dot(relativVel, n);
//
//    if (velAlongNormal > 0.0f) return;  // 이미 서로 멀어지는 중 (충돌 스킵)
//
//    float e = 1.0f; // 탄성계수
//
//    float j = -(1.0f + e) * velAlongNormal; // 임펄스 j (튕겨지는 세기)
//    j /= invSum;
//
//    FVector impulse = Mul(n, j);
//
//    velocityA = Sub(velocityA, Mul(impulse, inv_mA));
//    velocityB = Add(velocityB, Mul(impulse, inv_mB));
//
//    // 실제 velocity에 반영
//    ballA.Velocity = velocityA;
//    ballB.Velocity = velocityB;
//}
//
//// 공들의 충돌 처리 함수
//void BallsCollision(UPrimitive** PrimitiveList, int NumPrimitives)
//{
//    // 공과 공 사이 충돌
//    for (int i = 0; i < NumPrimitives; ++i)
//    {
//        UBall* ballA = static_cast<UBall*>(PrimitiveList[i]);
//
//        for (int j = i + 1; j < NumPrimitives; ++j)
//        {
//            UBall* ballB = static_cast<UBall*>(PrimitiveList[j]);
//
//            ResolveBallCollision(*ballA, *ballB);
//        }
//    }
//}

//// 공의 총 개수
//int UBall::TotalNumBalls = 0;

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
    srand(GetTickCount64());

    /* 생성하는 코드를 여기에 추가합니다. */

    // Renderer Class를 생성
    URenderer renderer;

    // D3D11을 사용한 Renderer 생성
    renderer.Create(hWnd);

    // Renderer 생성 직후, Shader 생성
    renderer.CreateShader();

    // Constant Buffer 생성
    renderer.CreateConstantBuffer();
    renderer.CreateRectBuffer();
    // ImGui를 사용하기 위한 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

    // Renderer와 Shader 생성 이후, vertexBuffer 생성
    UINT NumVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
    ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));
    UINT NumVerticesBar = sizeof(bar_vertices) / sizeof(FVertexSimple);
    ID3D11Buffer* vertexBufferBar = renderer.CreateVertexBuffer(bar_vertices, sizeof(bar_vertices));


    // 반드시 UBall이 아닌 UPrimitive로 선언하여야 하며 바꾸면 안됩니다.
    //UPrimitive** PrimitiveList = nullptr;

    //int NumPrimitives = 0;          // 현재 관리 중인 공 개수
    //int TargetNumPrimitives = 1;    // UI로 조절하는 목표 공 개수

    //// 중력 적용 여부
    //bool bEnableGravity = true;

    //// 중력 가속도 (기본 -Y 방향)
    //const float gravityAccel = 9.8f;
    //FVector gravityVec(0.0f, -gravityAccel, 0.0f);


    // FPS 제한
    const int targetFPS = 60;
    const double targetFrameTime = 1000.0 / targetFPS; // 한 프레임의 목표 시간 (밀리초 단위)
    const float dt = 1.0f / static_cast<float>(targetFPS);

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;

    UBar Bar(FVector(0.0f, -0.95f, 0.0f), 0.7f, 0.3f, 0);
    UBall Ball;
    InitBall(Ball);

    UBlock* testBlocks[3] = {
    new UBlock(EBlockType::Normal,   EBlockColor::Red,    1),
    new UBlock(EBlockType::Hard,     EBlockColor::White, 1),
    new UBlock(EBlockType::Immortal, EBlockColor::White,   1),
    };
    testBlocks[0]->Init(-0.6f, 0.5f, 0.1f, 0.025f);
    testBlocks[1]->Init(0.0f, 0.5f, 0.1f, 0.025f);
    testBlocks[2]->Init(0.6f, 0.5f, 0.1f, 0.025f);

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
            if (msg.message == WM_KEYDOWN) {
                if (msg.wParam == VK_LEFT)  Bar.Direction = -1;
                if (msg.wParam == VK_RIGHT) Bar.Direction = 1;
            }
            else if (msg.message == WM_KEYUP) {
                Bar.Direction = 0; // 키를 떼면 멈춤
            }
		}

		////////////////////////////////////////////
		// 매번 실행되는 코드를 여기에 추가합니다.
        Bar.Update(dt);
        Ball.Update(dt);

        if (Ball.CheckCollision(&Bar))
        {
            Ball.BallBounceAtBar(Bar);
        }

        for (auto* b : testBlocks)
        {
            EBlockCollision CollisionState = Ball.CheckBlockCollision(*b);

            Ball.BallBounceAtBlock(CollisionState, *b);
        }

        // 준비 작업
        renderer.Prepare();
        renderer.PrepareShader();

        // 생성한 버텍스 버퍼를 넘겨 실질적인 렌더링 요청
        
        //// 중력 적용
        //if (bEnableGravity)
        //{
        //    for (int i = 0; i < NumPrimitives; ++i)
        //    {
        //        PrimitiveList[i]->ApplyGravity(dt, gravityVec);
        //    }
        //}

        // 업데이트
        //for (int i = 0; i < NumPrimitives; ++i)
        //{
        //    PrimitiveList[i]->Update(dt);
        //}

        //// 공과 공 사이의 충돌 계산
        //BallsCollision(PrimitiveList, NumPrimitives);

        // 렌더링
        //for (int i = 0; i < NumPrimitives; ++i)
        //{
        //    PrimitiveList[i]->Render(renderer);

        //    // 실제 Draw
        //    renderer.RenderPrimitive(vertexBufferSphere, NumVerticesSphere);
        //}
        Bar.Render(renderer);
        renderer.RenderPrimitive(vertexBufferBar, NumVerticesBar);

        Ball.Render(renderer);
 
        for (int i = 0; i < 3; ++i)
        {
            UBlock* b = testBlocks[i];
            if (!b->IsActive()) continue;

            renderer.UpdateConstant(
                FVector(b->CenterX, b->CenterY, 0.0f),
                b->HalfW,
                b->HalfH,
                b->GetColor()
            );
            renderer.RenderPrimitive(renderer.RectVB, 12);
        }
        ImGui_ImplDX11_NewFrame();      // 렌더러(D3D11) 쪽에서 ImGui 프레임 준비
        ImGui_ImplWin32_NewFrame();     // 플랫폼(Win32) 쪽에서 ImGui 프레임 준비
        ImGui::NewFrame();

        //ImGui_ImplDX11_NewFrame();      // 렌더러(D3D11) 쪽에서 ImGui 프레임 준비
        //ImGui_ImplWin32_NewFrame();     // 플랫폼(Win32) 쪽에서 ImGui 프레임 준비
        //ImGui::NewFrame();

        ///***** 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 ImGui::Render() 사이인 여기에 위치합니다. *****/
        //ImGui::Begin("Jungle Property Window");
        //
        //ImGui::Text("Hello Jungle World!");

        //ImGui::Checkbox("Gravity", &bEnableGravity);

        //ImGui::InputInt("Number of Balls", &TargetNumPrimitives);

        //if (TargetNumPrimitives < 1)
        //    TargetNumPrimitives = 1;

        //// 목표 개수를 반영
        //AdjustPrimitiveListTo(PrimitiveList, NumPrimitives, TargetNumPrimitives);

        //ImGui::End();
        ///*****                              (실제 ImGui UI를 코드로 그리는 구간)                         *****/

        //ImGui::Render();    // 지금까지 쌓아둔 UI 명령들을 렌더링용 데이터(Draw Data)로 변환
        //ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());    // 데이터(Draw Data)를 D3D11 Draw Call로 변환해서 실제로 그림

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
    //for (int i = 0; i < NumPrimitives; ++i)
    //{
    //    delete PrimitiveList[i];
    //    PrimitiveList[i] = nullptr;
    //}
    //delete[] PrimitiveList;
    //PrimitiveList = nullptr;
    //NumPrimitives = 0;

    //// ImGui 관련
    //ImGui_ImplDX11_Shutdown();
    //ImGui_ImplWin32_Shutdown();
    //ImGui::DestroyContext();

    // vertexBuffer 릴리즈
    renderer.ReleaseVertexBuffer(vertexBufferBar);

    // Constant Buffer 릴리즈
	renderer.CreateRectBuffer();
    renderer.ReleaseConstantBuffer();

    // 렌더러 소멸 직전, 쉐이더 소멸
    renderer.ReleaseShader();

    // 렌더러 소멸
    renderer.Release();

	return 0;
}