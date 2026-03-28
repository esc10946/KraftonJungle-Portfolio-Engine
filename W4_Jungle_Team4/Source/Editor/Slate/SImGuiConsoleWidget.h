#pragma once

#include "SWidget.h"
#include "ImGui/imgui.h"
#include <functional>

class SImGuiConsoleWidget : public SWidget
{
public:
	SImGuiConsoleWidget() = default;
	virtual ~SImGuiConsoleWidget() = default;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPaint() override;

	virtual FString GetWidgetType() const { return "SImGuiConsoleWidget"; }

	static void AddLog(const char* fmt, ...);

	void Clear();
	static void ClearHistory();

private:
	char InputBuf[256]{};
	static ImVector<char*> Messages;
	static ImVector<char*> History;
	int32 HistoryPos = -1;
	ImGuiTextFilter Filter;
	static bool AutoScroll;
	static bool ScrollToBottom;

	//Command Dispatch System
	using CommandFn = std::function<void(const TArray<FString>& args)>;
	TMap<FString, CommandFn> Commands;

	void RegisterCommand(const FString& Name, CommandFn Fn);
	void ExecCommand(const char* CommandLine);
	static int32 TextEditCallback(ImGuiInputTextCallbackData* Data);
};
