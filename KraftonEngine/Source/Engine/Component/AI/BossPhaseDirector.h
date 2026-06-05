#pragma once

#include "Component/ActorComponent.h"
#include "Core/Delegate.h"
#include "Object/FName.h"

class UPhaseComponent;

#include "Source/Engine/Component/AI/BossPhaseDirector.generated.h"

// =============================================================================
//  UBossPhaseDirector — 보스 페이즈 감독기 (엔진 코어 메커니즘, Phase 3)
// =============================================================================
//  보고서 "Boss Phase Director / Phase Rules". 보스의 UPhaseComponent 페이즈 전환을
//  감독한다. 페이즈가 바뀌면:
//   - 짧은 슈퍼아머 "페이즈 전환" 윈도우를 열어 연출 중 끊기지 않게 한다(CinematicGate).
//   - 체간을 리셋해 새 페이즈를 신선한 상태로 시작.
//   - 페이즈별 공격성(PhaseAggression)을 Blackboard 에 기록 → Utility 선택기가
//     높은 페이즈에서 압박/돌진/위험공격 가치를 키운다.
// =============================================================================

DECLARE_MULTICAST_DELEGATE_TwoParams(FBossPhaseDirectorSignature, class UBossPhaseDirector*, int32);

UCLASS()
class UBossPhaseDirector : public UActorComponent
{
public:
    GENERATED_BODY()
    UBossPhaseDirector()           = default;
    ~UBossPhaseDirector() override = default;

    void BeginPlay() override;
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

    UFUNCTION(Pure, Category="Boss|Phase")
    float GetPhaseAggression(int32 Phase) const;
    UFUNCTION(Pure, Category="Boss|Phase")
    int32 GetTrackedPhase() const { return TrackedPhase; }
    UFUNCTION(Pure, Category="Boss|Phase")
    bool IsInPhaseTransition() const;

    UPROPERTY(Edit, Save, Category="Boss|Phase", DisplayName="Base Aggression", Min=0.0f, Max=10.0f, Speed=0.05f)
    float BaseAggression = 1.0f;

    UPROPERTY(Edit, Save, Category="Boss|Phase", DisplayName="Aggression Per Phase", Min=0.0f, Max=5.0f, Speed=0.05f)
    float AggressionPerPhase = 0.25f;

    UPROPERTY(Edit, Save, Category="Boss|Phase", DisplayName="Phase Transition Super Armor Seconds", Min=0.0f, Max=10.0f, Speed=0.05f)
    float PhaseTransitionSuperArmorSeconds = 1.2f;

    UPROPERTY(Edit, Save, Category="Boss|Phase", DisplayName="Reset Posture On Phase Change")
    bool bResetPostureOnPhaseChange = true;

    FBossPhaseDirectorSignature OnPhaseTransition;

private:
    void HandlePhaseChanged(UPhaseComponent* Component, int32 OldPhase, int32 NewPhase);
    void ApplyPhase(int32 Phase, bool bIsTransition);

    int32 TrackedPhase           = 1;
    float TransitionUntilSeconds = 0.0f;
    bool  bSuperArmorEngaged     = false;
};
