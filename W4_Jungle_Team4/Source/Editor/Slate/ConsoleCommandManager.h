#pragma once

#include "Core/Singleton.h"
#include "Core/EngineTypes.h"
#include <functional>

class SImGuiConsoleWidget;

class FConsoleCommandManager : public TSingleton<FConsoleCommandManager>
{
	using FCommandCallback = std::function<void(const TArray<FString>& args)>;
public:
	void Initialize(SImGuiConsoleWidget* InConsoleWidget);

	void RegisterCommand(const FString& Name, FCommandCallback Fn);
	void ExecCommand(const FString& FullCommandLine);

	TMap<FString, FCommandCallback>& GetCommands() { return Commands; }

private:
	TMap<FString, FCommandCallback> Commands;

	SImGuiConsoleWidget* ConsoleWidget = nullptr;
};
