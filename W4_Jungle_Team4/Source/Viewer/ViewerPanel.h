#pragma once

class FRenderer;
class UViewerEngine;
class FWindowsWindow;
class FViewerViewportClient;

class FViewerPanel
{
public:
	void Create(FWindowsWindow* InWindow, FRenderer& InRenderer);
	void Release();

	void BeginFrame();
	void Render(float DeltaTime);

	void SetViewportClient(FViewerViewportClient* InClient) { ViewportClient = InClient; }

private:
	FWindowsWindow* Window;
	FViewerViewportClient* ViewportClient = nullptr;
};