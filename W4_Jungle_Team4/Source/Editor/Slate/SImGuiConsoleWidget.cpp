#include "SImGuiConsoleWidget.h"
#include "ConsoleCommandManager.h"
#include <cstdarg>
#include <sstream>

SImGuiConsoleWidget::SImGuiConsoleWidget()
{
	ConsoleCommandManager::Get().Initialize(this);
}

void SImGuiConsoleWidget::Tick(float DeltaTime)
{
	SWidget::Tick(DeltaTime);
}

void SImGuiConsoleWidget::OnPaint()
{
	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

	ImGui::SetNextWindowPos(ImVec2((float)Rect.X, (float)Rect.Y));
	ImGui::SetNextWindowSize(ImVec2((float)Rect.Width, (float)Rect.Height));

	if (!ImGui::Begin("Console", nullptr, WindowFlags))
	{
		ImGui::End();
		return;
	}


	if (ImGui::SmallButton("Clear")) { Clear(); }

	ImGui::Separator();

	//// Options menu
	if (ImGui::BeginPopup("Options"))
	{
		ImGui::Checkbox("Auto-scroll", &AutoScroll);
		ImGui::EndPopup();
	}

	// Options, Filter
	ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_Tooltip);
	if (ImGui::Button("Options"))
		ImGui::OpenPopup("Options");
	ImGui::SameLine();
	Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
	ImGui::Separator();

	const float FooterHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -FooterHeight), false, ImGuiWindowFlags_HorizontalScrollbar)) {
		for (auto& Item : Messages) {
			if (!Filter.PassFilter(Item)) continue;

			ImVec4 Color;
			bool bHasColor = false;
			if (strncmp(Item, "[ERROR]", 7) == 0) {
				Color = ImVec4(1, 0.4f, 0.4f, 1);
				bHasColor = true;
			}
			else if (strncmp(Item, "[WARN]", 6) == 0) {
				Color = ImVec4(1, 0.8f, 0.2f, 1);
				bHasColor = true;
			}
			else if (strncmp(Item, "#", 1) == 0) {
				Color = ImVec4(1, 0.8f, 0.6f, 1);
				bHasColor = true;
			}

			if (bHasColor) {
				ImGui::PushStyleColor(ImGuiCol_Text, Color);
			}
			ImGui::TextUnformatted(Item);
			if (bHasColor) {
				ImGui::PopStyleColor();
			}
		}

		if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
			ImGui::SetScrollHereY(1.0f);
		}
		ScrollToBottom = false;
	}
	ImGui::EndChild();
	ImGui::Separator();

	// Input line
	bool bReclaimFocus = false;
	ImGuiInputTextFlags Flags = ImGuiInputTextFlags_EnterReturnsTrue
		| ImGuiInputTextFlags_EscapeClearsAll
		| ImGuiInputTextFlags_CallbackHistory
		| ImGuiInputTextFlags_CallbackCompletion;
	if (ImGui::InputText("Input", InputBuf, sizeof(InputBuf), Flags, &TextEditCallback, this)) {
		ExecCommand(InputBuf);
		strcpy_s(InputBuf, "");
		bReclaimFocus = true;
	}

	ImGui::SetItemDefaultFocus();
	if (bReclaimFocus) {
		ImGui::SetKeyboardFocusHere(-1);
	}

	ImGui::End();
}

void SImGuiConsoleWidget::AddLog(const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	Messages.push_back(_strdup(buf));
	if (AutoScroll) ScrollToBottom = true;
}

void SImGuiConsoleWidget::Clear()
{
	for (int32 i = 0; i < Messages.Size; i++) free(Messages[i]);
	Messages.clear();
}

void SImGuiConsoleWidget::ClearHistory()
{
	for (int32 i = 0; i < History.Size; i++) free(History[i]);
	History.clear();
}

void SImGuiConsoleWidget::RegisterCommand(const FString& Name, CommandFn Fn)
{
	ConsoleCommandManager::Get().RegisterCommand(Name, Fn);
}

void SImGuiConsoleWidget::ExecCommand(const char* CommandLine)
{
	AddLog("# %s\n", CommandLine);
	History.push_back(_strdup(CommandLine));
	HistoryPos = -1;

	TArray<FString> Tokens;
	std::istringstream Iss(CommandLine);
	FString Token;
	while (Iss >> Token) Tokens.push_back(Token);
	if (Tokens.empty()) return;

	ConsoleCommandManager::Get().ExecCommand(CommandLine);
}

int32 SImGuiConsoleWidget::TextEditCallback(ImGuiInputTextCallbackData* Data)
{
	SImGuiConsoleWidget* Console = (SImGuiConsoleWidget*)Data->UserData;

	if (Data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
		const int32 PrevPos = Console->HistoryPos;
		if (Data->EventKey == ImGuiKey_UpArrow) {
			if (Console->HistoryPos == -1)
				Console->HistoryPos = Console->History.Size - 1;
			else if (Console->HistoryPos > 0)
				Console->HistoryPos--;
		}
		else if (Data->EventKey == ImGuiKey_DownArrow) {
			if (Console->HistoryPos != -1 &&
				++Console->HistoryPos >= Console->History.Size)
				Console->HistoryPos = -1;
		}
		if (PrevPos != Console->HistoryPos) {
			const char* HistoryStr = (Console->HistoryPos >= 0)
				? Console->History[Console->HistoryPos] : "";
			Data->DeleteChars(0, Data->BufTextLen);
			Data->InsertChars(0, HistoryStr);
		}
	}

	if (Data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
		// Find last word typed
		const char* WordEnd = Data->Buf + Data->CursorPos;
		const char* WordStart = WordEnd;
		while (WordStart > Data->Buf && WordStart[-1] != ' ')
			WordStart--;

		// Collect matches from registered commands
		ImVector<const char*> Candidates;
		for (auto& Pair : ConsoleCommandManager::Get().GetCommands()) {
			const FString& Name = Pair.first;
			if (strncmp(Name.c_str(), WordStart, WordEnd - WordStart) == 0)
				Candidates.push_back(Name.c_str());
		}

		if (Candidates.Size == 1) {
			Data->DeleteChars(static_cast<int32>(WordStart - Data->Buf), static_cast<int32>(WordEnd - WordStart));
			Data->InsertChars(Data->CursorPos, Candidates[0]);
			Data->InsertChars(Data->CursorPos, " ");
		}
		else if (Candidates.Size > 1) {
			Console->AddLog("Possible completions:\n");
			for (auto& C : Candidates) Console->AddLog("  %s\n", C);
		}
	}

	return 0;
}

ImVector<char*> SImGuiConsoleWidget::Messages;
ImVector<char*> SImGuiConsoleWidget::History;

bool SImGuiConsoleWidget::AutoScroll = true;
bool SImGuiConsoleWidget::ScrollToBottom = true;
