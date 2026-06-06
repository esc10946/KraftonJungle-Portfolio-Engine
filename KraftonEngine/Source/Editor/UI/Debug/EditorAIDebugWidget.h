#pragma once

#include "Editor/UI/EditorWidget.h"
#include "Core/Types/CoreTypes.h"
#include "Object/FName.h"

class AEnemyCharacter;

// 선택된 AEnemyCharacter 의 세키로식 전투 AI 계층(Blackboard / Perception /
// Utility Reasoner / Decision Trace / HFSM / Squad)을 라이브로 시각화하는 read-only 패널.
// 보고서가 최우선으로 꼽은 "AI Frame Debugger / score heatmap / sensor overlay".
//
// 그래프 4종: ① 결정 점수 막대(전 후보 히트맵) ② 의심도 시계열 ③ 상태전이 타임라인
//            ④ 스쿼드 공격슬롯 링.  + 뷰포트 센서 오버레이(시야콘/LOS/기억점).
//
// Visibility 는 FEditorSettings::UI.bAIDebug 가 단독 source of truth — MainPanel 에서 조건부 호출.
class FEditorAIDebugWidget : public FEditorWidget
{
public:
	void Render(float DeltaTime) override;

private:
	// 타임라인 한 세그먼트 = 한 상태에 머문 [Start,End] 구간(에디터 전용 샘플링 기록).
	struct FStateSegment
	{
		FName Label;
		float Start    = 0.0f;
		float End      = 0.0f;
		int32 ColorKey = 0;
	};

	void SampleHistory(AEnemyCharacter* Enemy, float DeltaTime);
	void ResetTracking();
	void DrawSensorOverlay(AEnemyCharacter* Enemy);

	// 체력/체간/의심도 시계열 (ImGui::PlotLines 용 롤링 버퍼).
	TArray<float> HealthHistory;
	TArray<float> PostureHistory;
	TArray<float> SuspicionHistory;

	// HFSM / Awareness 상태전이 타임라인 (완료된 세그먼트만 보관, 진행 중 구간은 라이브로 그림).
	TArray<FStateSegment> HfsmSegments;
	TArray<FStateSegment> AwareSegments;
	FName  LastHfsmState;
	float  HfsmSegStart  = 0.0f;
	int32  LastAwareState = -1;
	float  AwareSegStart = 0.0f;
	float  TimelineClock = 0.0f; // Render 누적 시간축(초)

	bool        bSensorOverlay = false;
	const void* LastTrackedActor = nullptr;
};
