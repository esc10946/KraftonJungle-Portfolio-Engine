#include <windows.h>

// D3D ?ъ슜???꾩슂???쇱씠釉뚮윭由щ뱾??留곹겕
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D ?ъ슜???꾩슂???ㅻ뜑?뚯씪?ㅼ쓣 ?ы븿
#include "URenderer.h"
#include "UDiagram.h"
#include "UBar.h"
#include "Bar.h"
#include "UBall.h"
#include "Sphere.h"
#include "Util.h"
#include "USceneManager.h"
#include "UScene.h"

// ImGui 愿???ㅻ뜑
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"
#include "UInputManager.h"
#include "USoundManager.h"
#include "UGameManager.h"
#include <crtdbg.h> 

#pragma comment(lib, "runtimeobject.lib")
// ?덈룄?곗쓽 ?낅젰 ?대깽?몃? ImGui???꾨떖?섍퀬, ImGui媛 ?ъ슜?덈뒗吏 ?щ?瑜??뚮젮二쇰뒗 ?⑥닔
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/// <summary>
/// ?덈룄?곗뿉 ?대깽?멸? 諛쒖깮??寃쎌슦 泥섎━?섎뒗 肄쒕갚 ?⑥닔
/// </summary>
/// <param name="hWnd"> ?대뼡 ?덈룄?곗뿉 ???硫붿떆吏?몄? </param>
/// <param name="message"> ?대뼡 ?대깽?몄씤吏 (醫낅즺, ???낅젰, 留덉슦???낅젰 ?? </param>
/// <param name="wParam"> 硫붿떆吏???몃젮?ㅻ뒗 異붽? ?뺣낫 </param>
/// <param name="lParam"> 硫붿떆吏???몃젮?ㅻ뒗 異붽? ?뺣낫 </param>
/// <returns></returns>
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGui???대깽???낅젰?몄? ?먮퀎
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

	switch (message)
	{
	case WM_DESTROY:
		// ?덈룄?곌? ?ロ옄 ??
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam); // ?덈룄??湲곕낯 泥섎━ 諛⑹떇
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
   // _CrtSetBreakAlloc(75);
 
	// ?덈룄???대옒???대쫫
	WCHAR WindowClass[] = L"JungleWindowClass";

	// ?덈룄????댄?諛붿뿉 ?쒖떆???대쫫
	WCHAR Title[] = L"Game Tech Lab";

	// ???대옒???대쫫(WindowClass)?쇰줈 ?앹꽦??紐⑤뱺 ?덈룄?곗쓽 硫붿떆吏??WndProc()?먯꽌 泥섎━?쒕떎.
	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// ?꾩뿉??留뚮뱺 wndclass ?쇰뒗 ?덈룄???대옒?ㅻ? ?깅줉
	RegisterClassW(&wndclass);

	// 1024 x 1024 ?ш린???덈룄???앹꽦
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
		nullptr, nullptr, hInstance, nullptr);

	bool bIsExit = false;

    /* ?앹꽦?섎뒗 肄붾뱶瑜??ш린??異붽??⑸땲?? */

    // Renderer Class瑜??앹꽦
    URenderer renderer;

    // D3D11???ъ슜??Renderer ?앹꽦
    renderer.Create(hWnd);

    // Renderer ?앹꽦 吏곹썑, Shader ?앹꽦
    renderer.CreateShader();

    // Constant Buffer ?앹꽦
    renderer.CreateConstantBuffer();
    renderer.CreateRectBuffer();
    //render?먯꽌 珥덇린??
    // Renderer? Shader ?앹꽦 ?댄썑, vertexBuffer ?앹꽦
    renderer.NumVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
    renderer.vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));
    renderer.NumVerticesBar = sizeof(bar_vertices) / sizeof(FVertexSimple);
    renderer.vertexBufferBar = renderer.CreateVertexBuffer(bar_vertices, sizeof(bar_vertices));
    //renderer.vertexBufferRect = renderer.CreateVertexBuffer(bar_vertices, sizeof(bar_vertices));
    // ImGui瑜??ъ슜?섍린 ?꾪븳 珥덇린??
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);
    
    // FPS ?쒗븳
    const int targetFPS = 60;
    const double targetFrameTime = 1000.0 / targetFPS; // ???꾨젅?꾩쓽 紐⑺몴 ?쒓컙 (諛由ъ큹 ?⑥쐞)
    const float dt = 1.0f / static_cast<float>(targetFPS);

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;

    USoundManager::GetInstance().Init();
    //寃뚯엫??珥덇린??
    USceneManager& sceneManager = USceneManager::GetInstance();
    sceneManager.LoadScene(ESceneType::Title);


	// Main Loop (Quit Message媛 ?ㅼ뼱?ㅺ린 ?꾧퉴吏 ?꾨옒 Loop瑜?臾댄븳???ㅽ뻾?섍쾶 ??
	while (bIsExit == false)
	{
        // 硫붿씤 猷⑦봽 ?쒖옉 ?쒓컙 湲곕줉
        QueryPerformanceCounter(&startTime);

		MSG msg;

		// 泥섎━??硫붿떆吏媛 ???댁긽 ?놁쓣??源뚯? ?섑뻾
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// ???낅젰 硫붿떆吏瑜?踰덉뿭
			TranslateMessage(&msg);

			// 硫붿떆吏瑜??곸젅???덈룄???꾨줈?쒖????꾨떖, 硫붿떆吏媛 ?꾩뿉???깅줉??WndProc ?쇰줈 ?꾨떖??
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)             // 醫낅즺 ??
			{
				bIsExit = true;
				break;
			}

                //if (msg.wParam == VK_LEFT)
                //    sceneManager.LoadScene(ESceneType::Title);
                //if (msg.wParam == VK_RIGHT)
                //    sceneManager.LoadScene(ESceneType::InGame);

                //if (msg.wParam == VK_RIGHT) Bar.Direction = 1;
            //}
            //else if (msg.message == WM_KEYUP) {
            //    Bar.Direction = 0; // ?ㅻ? ?쇰㈃ 硫덉땄
            //}
		}

        USoundManager::GetInstance().Update();
        UInputManager::GetInstance()->Update();
		////////////////////////////////////////////
		// 留ㅻ쾲 ?ㅽ뻾?섎뒗 肄붾뱶瑜??ш린??異붽??⑸땲??
        UScene* currentScene = sceneManager.GetCurrentScene();
        currentScene->Update(dt);

        // 以鍮??묒뾽
        renderer.Prepare();
        renderer.PrepareShader();
        currentScene->Render(renderer);
        currentScene->UIRender();

        // ??洹몃졇?쇰㈃ 踰꾪띁 ?ㅼ솑
        
        renderer.SwapBuffer();

		////////////////////////////////////////////

        // 猷⑦봽 ?ㅽ뻾 ?쒓컙???곕씪 FPS ?쒗븳
        do
        {
            Sleep(0);

            // 猷⑦봽 醫낅즺 ?쒓컙 湲곕줉
            QueryPerformanceCounter(&endTime);

            // ???꾨젅?꾩씠 ?뚯슂???쒓컙 怨꾩궛 (諛由ъ큹 ?⑥쐞濡?蹂??
            elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

        } while (elapsedTime < targetFrameTime);
	}

    USoundManager::GetInstance().Release();
    USceneManager::GetInstance().Release();
    UInputManager::GetInstance()->Release();
    UGameManager::GetInstance()->Release();

    // vertexBuffer 由대━利?
    renderer.ReleaseVertexBuffer();

    // Constant Buffer 由대━利?
    renderer.ReleaseRectBuffer();
    renderer.ReleaseConstantBuffer();

    // ?뚮뜑???뚮㈇ 吏곸쟾, ?먯씠???뚮㈇
    renderer.ReleaseShader();

    ID3D11Debug* debugDevice = nullptr;
    HRESULT hr = renderer.Device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debugDevice);

    if (SUCCEEDED(hr))
    {
        debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

        debugDevice->Release();
    }

    // ?뚮뜑???뚮㈇
    renderer.Release();
 
	return 0;
}
