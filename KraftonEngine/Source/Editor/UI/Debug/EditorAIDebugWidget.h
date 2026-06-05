#pragma once

#include "Editor/UI/EditorWidget.h"
#include "Core/Types/CoreTypes.h"

class AEnemyCharacter;

// 선택된 AEnemyCharacter 의 세키로식 전투 AI 계층(Blackboard / Perception /
// Utility Reasoner / Decision Trace / HFSM)을 라이브로 시각화하는 read-only 패널.
// 보고서가 최우선으로 꼽은 "AI Frame Debugger / score heatmap / sensor overlay".
//
// Visibility 는 FEditorSettings::UI.bAIDebug 가 단독 source of truth — MainPanel 에서 조건부 호출.
class FEditorAIDebugWidget : public FEditorWidget
{
public:
	void Render(float DeltaTime) override;

private:
	void SampleHistory(AEnemyCharacter* Enemy);

	// 체력/체간 시계열 (ImGui::PlotLines 용 롤링 버퍼).
	TArray<float> HealthHistory;
	TArray<float> PostureHistory;
	const void*   LastTrackedActor = nullptr;
};
