#pragma once

#include "Editor/UI/EditorViewportOverlayWidget.h"
#include "Editor/UI/EditorStatWidget.h"

class FRenderer;
class UEditorEngine;
class FWindowsWindow;

class FEditorMainPanel
{
public:
	void Create(FWindowsWindow* InWindow, FRenderer& InRenderer, UEditorEngine* InEditorEngine);
	void Release();

	void BeginFrame();
	void Render(float DeltaTime);
	void Update();

private:
	FWindowsWindow* Window;
	FEditorViewportOverlayWidget ViewportOverlayWidget;
	FEditorStatWidget StatWidget;
	//TODO: 여기에 staticWidget 도 추가하기
};
