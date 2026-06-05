#pragma once

#include "Component/ActorComponent.h"
#include "Core/Types/CoreTypes.h"
#include "Object/FName.h"

#include "Source/Engine/Component/AI/AIDecisionTraceComponent.generated.h"

// =============================================================================
//  UAIDecisionTraceComponent — 의사결정 트레이스 레코더 (엔진 코어 메커니즘)
// =============================================================================
//  보고서 "Decision Trace / Combat Trace Recorder". 매 think 스텝의 결정을
//  순환 버퍼에 기록해 AI 디버거가 시간순 로그로 보여 준다. 무엇을 후보로 두었고
//  (top-3 점수), 무엇을 골랐으며, 그때 상태/자극이 어땠는지를 남긴다.
// =============================================================================

struct FDecisionTraceRecord
{
    int64 Tick          = 0;
    FName State;
    FName Chosen;
    FName TopName[3];
    float TopScore[3]   = { 0.0f, 0.0f, 0.0f };
    int32 CandidateCount = 0;
    int32 StimulusCount  = 0;
};

UCLASS()
class UAIDecisionTraceComponent : public UActorComponent
{
public:
    GENERATED_BODY()
    UAIDecisionTraceComponent()           = default;
    ~UAIDecisionTraceComponent() override = default;

    // 형제 Reasoner/Brain/Perception 에서 스냅샷을 모아 한 레코드를 추가한다.
    UFUNCTION(Callable, Category="AI|Trace")
    void RecordDecision();

    UFUNCTION(Callable, Category="AI|Trace")
    void ClearRecords();

    UPROPERTY(Edit, Save, Category="AI|Trace", DisplayName="Max Records", Min=8.0f, Max=2048.0f, Speed=1.0f)
    int32 MaxRecords = 256;

    // ── 디버거/내부용 직접 접근 (가장 최근이 front) ──
    const TArray<FDecisionTraceRecord>& GetRecords() const { return Records; }

private:
    TArray<FDecisionTraceRecord> Records;
    int64                        NextTick = 0;
};
