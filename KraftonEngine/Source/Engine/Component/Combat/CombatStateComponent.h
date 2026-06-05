#pragma once

#include "Component/ActorComponent.h"
#include "Component/Combat/CombatTypes.h"
#include "Core/Delegate.h"

#include "Source/Engine/Component/Combat/CombatStateComponent.generated.h"

class AActor;

DECLARE_MULTICAST_DELEGATE_TwoParams(FCombatStaggerSignature, class UCombatStateComponent*, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FCombatStateSignature, class UCombatStateComponent*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FCombatPerilousSignature, class UCombatStateComponent*, EPerilousType);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FCombatDeflectSignature, class UCombatStateComponent*, EDeflectGrade, AActor*);

UCLASS()
class UCombatStateComponent : public UActorComponent
{
public:
	GENERATED_BODY()
	UCombatStateComponent() = default;
	~UCombatStateComponent() override = default;

	void BeginPlay() override;
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

	UFUNCTION(Pure, Category="Combat|State")
	bool CanReceiveDamage() const { return bDamageEnabled && !bInvincible; }
	UFUNCTION(Callable, Category="Combat|State")
	void SetDamageEnabled(bool bEnabled) { bDamageEnabled = bEnabled; }
	UFUNCTION(Callable, Category="Combat|State")
	void SetInvincible(bool bInInvincible) { bInvincible = bInInvincible; }
	UFUNCTION(Callable, Category="Combat|State")
	void SetSuperArmor(bool bInSuperArmor) { bSuperArmor = bInSuperArmor; }

	UFUNCTION(Pure, Category="Combat|State")
	bool IsInvincible() const { return bInvincible; }
	UFUNCTION(Pure, Category="Combat|State")
	bool HasSuperArmor() const { return bSuperArmor; }
	UFUNCTION(Pure, Category="Combat|State")
	bool IsStaggered() const { return bStaggered; }

	UFUNCTION(Callable, Category="Combat|Poise")
	bool ApplyPoiseDamage(float PoiseDamage);
	UFUNCTION(Callable, Category="Combat|Poise")
	void ResetPoise();
	UFUNCTION(Callable, Category="Combat|Poise")
	void StartStagger(float Duration);
	UFUNCTION(Callable, Category="Combat|Poise")
	void StopStagger();
	UFUNCTION(Pure, Category="Combat|Poise")
	float GetPoiseRatio() const;

	// ── Attack threat broadcast ──
	// 공격을 시작한 액터가 자신의 CombatState 에 "지금 공격 중" 을 표시한다(MarkAttacking).
	// 적 AI 는 타깃의 CombatState 를 폴링해 IsAttacking / GetAttackThreatRemaining 으로
	// 회피·패링 타이밍을 잡는다. 게임시간 기준 self-expiring 이라 Tick 의존이 없다.
	UFUNCTION(Callable, Category="Combat|Threat")
	void MarkAttacking(float WindowSeconds);
	UFUNCTION(Pure, Category="Combat|Threat")
	bool IsAttacking() const;
	UFUNCTION(Pure, Category="Combat|Threat")
	float GetAttackThreatRemaining() const;

	// ── 위험공격(perilous) 표시 (Phase 2) ──
	// 공격자가 위험공격 active 동안 자신을 위험으로 표시. 방어자 AI 는 타깃의 이것을 폴링해
	// 종류별 대응(찌르기=탄기/간파, 하단=점프, 잡기=이탈)을 고른다. 게임시간 self-expiring.
	UFUNCTION(Callable, Category="Combat|Perilous")
	void MarkPerilous(EPerilousType InType, float Duration);
	UFUNCTION(Pure, Category="Combat|Perilous")
	bool IsPerilousActive() const;
	UFUNCTION(Pure, Category="Combat|Perilous")
	EPerilousType GetActivePerilousType() const;
	UFUNCTION(Pure, Category="Combat|Perilous")
	int32 GetActivePerilousTypeInt() const { return static_cast<int32>(GetActivePerilousType()); }

	// ── 탄기(deflect) (Phase 2) ──
	// 방어자가 들어오는 공격을 받아넘기기 위해 짧은 윈도우를 연다. 윈도우 안에서 피격되면
	// HealthComponent 가 ConsumeDeflect 로 등급 산정 → 체력/체간 피해 무효 + 공격자 체간 반사.
	UFUNCTION(Callable, Category="Combat|Deflect")
	void OpenDeflectWindow(float Seconds);
	UFUNCTION(Pure, Category="Combat|Deflect")
	bool IsDeflecting() const;
	UFUNCTION(Callable, Category="Combat|Deflect")
	EDeflectGrade ConsumeDeflect(AActor* Attacker);
	UFUNCTION(Pure, Category="Combat|Deflect")
	int32 GetLastDeflectGradeInt() const { return static_cast<int32>(LastDeflectGrade); }

	UFUNCTION(Callable, Category="Combat|Team")
	void SetTeam(ECombatTeam InTeam) { Team = InTeam; }
	UFUNCTION(Pure, Category="Combat|Team")
	ECombatTeam GetTeam() const { return Team; }
	UFUNCTION(Pure, Category="Combat|Team")
	bool IsHostileTo(const UCombatStateComponent* Other) const;

	UPROPERTY(Edit, Save, Category="Combat|Team", DisplayName="Team", Enum=ECombatTeam)
	ECombatTeam Team = ECombatTeam::Neutral;

	UPROPERTY(Edit, Save, Category="Combat|Damage", DisplayName="Damage Enabled")
	bool bDamageEnabled = true;

	UPROPERTY(Edit, Save, Category="Combat|Damage", DisplayName="Invincible")
	bool bInvincible = false;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Super Armor")
	bool bSuperArmor = false;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Max Poise", Min=0.0f, Max=100000.0f, Speed=1.0f)
	float MaxPoise = 100.0f;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Current Poise", Min=0.0f, Max=100000.0f, Speed=1.0f)
	float CurrentPoise = 100.0f;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Poise Recovery Per Second", Min=0.0f, Max=100000.0f, Speed=1.0f)
	float PoiseRecoveryPerSecond = 20.0f;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Default Stagger Duration", Min=0.0f, Max=10.0f, Speed=0.05f)
	float DefaultStaggerDuration = 0.45f;

	// 체력이 낮을수록 체간 회복이 느려진다(보고서: 체력-체간 회복 연계). 회복 배율 = lerp.
	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Posture Recovery Scales With Health")
	bool bPostureRecoveryScalesWithHealth = true;

	UPROPERTY(Edit, Save, Category="Combat|Deflect", DisplayName="Deflect Window Seconds", Min=0.0f, Max=2.0f, Speed=0.01f)
	float DeflectWindowSeconds = 0.18f;

	UPROPERTY(Edit, Save, Category="Combat|Deflect", DisplayName="Deflect Reflect Poise", Min=0.0f, Max=100000.0f, Speed=0.5f)
	float DeflectReflectPoise = 25.0f;

	FCombatStaggerSignature OnStaggerStarted;
	FCombatStateSignature OnStaggerEnded;
	FCombatPerilousSignature OnPerilousCue;
	FCombatDeflectSignature OnDeflectResolved;

private:
	float GetHealthRatioSafe() const;

	bool bStaggered = false;
	float StaggerRemainingTime = 0.0f;
	float AttackThreatUntilSeconds = 0.0f;

	// 위험공격 표시(게임시간 만료).
	EPerilousType ActivePerilousType = EPerilousType::None;
	float PerilousUntilSeconds = 0.0f;

	// 탄기 윈도우(게임시간).
	float DeflectUntilSeconds = 0.0f;
	float DeflectOpenedAtSeconds = 0.0f;
	float DeflectWindowLen = 0.0f;
	EDeflectGrade LastDeflectGrade = EDeflectGrade::None;
};
