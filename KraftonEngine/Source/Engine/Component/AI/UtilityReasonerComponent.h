#pragma once

#include "Component/ActorComponent.h"
#include "Core/Types/CoreTypes.h"
#include "Object/FName.h"

class UEnemyAttackComponent;
class UAIBlackboardComponent;

#include "Source/Engine/Component/AI/UtilityReasonerComponent.generated.h"

// =============================================================================
//  UUtilityReasonerComponent — 연속값 Utility 선택기 (엔진 코어 메커니즘)
// =============================================================================
//  보고서 "Utility Selector" 계층. 기존 UEnemyAttackComponent 의 가중 난수 선택을
//  "투명한 ArgMax + 점수 분해"로 일반화한다. 거리/각도/위협/체간/페이즈/반복을
//  연속 곡선으로 곱해 후보별 최종 점수를 내고, 분해 결과를 그대로 보관해
//  AI 디버거가 막대 그래프로 시각화한다(보고서: score heatmap).
//
//  입력은 전부 Blackboard 에서 읽는다 — 센서/전투 계층과 직접 결합하지 않는다.
// =============================================================================

// 후보 한 개의 점수 분해(곱셈 항). 디버거가 이 필드들을 그대로 그린다.
struct FUtilityScoreBreakdown
{
    float Base       = 0.0f;
    float Range      = 1.0f;
    float Angle      = 1.0f;
    float Threat     = 1.0f;
    float Posture    = 1.0f;
    float Phase      = 1.0f;
    float Repetition = 1.0f;
    float Final      = 0.0f;
};

struct FUtilityCandidate
{
    FName                 ActionId;
    FUtilityScoreBreakdown Breakdown;
    bool                  bChosen = false;
};

UCLASS()
class UUtilityReasonerComponent : public UActorComponent
{
public:
    GENERATED_BODY()
    UUtilityReasonerComponent()           = default;
    ~UUtilityReasonerComponent() override = default;

    // 소유 액터의 공격 카탈로그를 후보로, Blackboard 입력으로 점수화하여 최고점을
    // 고른다. 선택 결과/분해를 보관하고 선택된 ActionId(없으면 None)를 반환한다.
    UFUNCTION(Callable, Category="AI|Utility")
    FName SelectBestAction();

    UFUNCTION(Pure, Category="AI|Utility")
    bool HasViableAction() const { return ChosenIndex >= 0; }
    UFUNCTION(Pure, Category="AI|Utility")
    FName GetChosenAction() const;

    UPROPERTY(Edit, Save, Category="AI|Utility", DisplayName="Act Threshold", Min=0.0f, Max=100.0f, Speed=0.01f)
    float ActThreshold = 0.05f;

    // ── 디버거/내부용 직접 접근 ──
    const TArray<FUtilityCandidate>& GetCandidates() const { return Candidates; }
    int32                            GetChosenIndex() const { return ChosenIndex; }

private:
    TArray<FUtilityCandidate> Candidates;
    int32                     ChosenIndex = -1;
};
