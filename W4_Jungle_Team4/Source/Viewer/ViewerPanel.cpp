#include "ViewerPanel.h"

#include "Engine/Render/Renderer/Renderer.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Engine/Core/InputSystem.h"
#include "ViewerViewportClient.h"
#include "ViewerEngine.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

void FViewerPanel::Create(FWindowsWindow* InWindow, FRenderer& InRenderer, UViewerEngine* InEngine)
{
	ImGui::CreateContext();

	Window = InWindow;
	OwnerEngine = InEngine;

	ImGui_ImplWin32_Init((void*)InWindow->GetHWND());
	ImGui_ImplDX11_Init(InRenderer.GetFD3DDevice().GetDevice(), InRenderer.GetFD3DDevice().GetDeviceContext());
}

void FViewerPanel::Release()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void FViewerPanel::BeginFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void FViewerPanel::Render(float DeltaTime)
{
	ImGuiIO& IO = ImGui::GetIO();
	// ImGui가 마우스/키보드 쓰는지 InputSystem에 알려주기
	InputSystem::Get().GetGuiInputState().bUsingMouse = IO.WantCaptureMouse;
	InputSystem::Get().GetGuiInputState().bUsingKeyboard = IO.WantCaptureKeyboard;

	ImGui::Begin("OBJ Viewer");

	ImGui::Text("Left Click  : Orbit");
	ImGui::Text("Right Click : Pan");
	ImGui::Text("Wheel       : Zoom");

	ImGui::Separator();

	if (ImGui::Button("Open OBJ File"))
	{
		OPENFILENAMEA ofn = {};
		char filePath[256] = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = "OBJ Files\0*.obj\0All Files\0*.*\0";
		ofn.lpstrFile = filePath;
		ofn.nMaxFile = sizeof(filePath);
		ofn.Flags = OFN_FILEMUSTEXIST;

		if (GetOpenFileNameA(&ofn) && ViewportClient)
		{
			OwnerEngine->LoadModel(filePath);
		}
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
