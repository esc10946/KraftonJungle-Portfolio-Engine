#include "ConsoleCommandManager.h"

#include "Slate/SImGuiConsoleWidget.h"
#include <sstream>

void FConsoleCommandManager::Initialize(SImGuiConsoleWidget* InConsoleWidget)
{
	ConsoleWidget = InConsoleWidget;

	RegisterCommand("clear", [this](const TArray<FString>& Args) {
		ConsoleWidget->Clear();
		});
}

void FConsoleCommandManager::RegisterCommand(const FString& Name, FCommandCallback Fn)
{
	Commands[Name] = Fn;
}

void FConsoleCommandManager::ExecCommand(const FString& FullCommandLine)
{
	TArray<FString> Tokens;
	std::istringstream Iss(FullCommandLine);
	FString Token;
	while (Iss >> Token) Tokens.push_back(Token);
	if (Tokens.empty()) return;
	auto It = Commands.find(Tokens[0]);
	if (It != Commands.end()) {
		It->second(Tokens);
	}
	else {
		SImGuiConsoleWidget::AddLog("[ERROR] Unknown command: '%s'\n", Tokens[0].c_str());
	}
}