#include "Component/Combat/CombatStateComponent.h"

#include "AI/CombatTargetRegistry.h"
#include "Component/Combat/HealthComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Math/MathUtils.h"

#include <algorithm>

namespace
{
	float GetOwnerGameTimeSeconds(const UActorComponent* Component)
	{
		AActor* OwnerActor = Component ? Component->GetOwner() : nullptr;
		UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
		return World ? World->GetGameTimeSeconds() : 0.0f;
	}
}

void UCombatStateComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	if (MaxPoise < 0.0f)
	{
		MaxPoise = 0.0f;
	}
	CurrentPoise = FMath::Clamp(CurrentPoise, 0.0f, MaxPoise);
	// 전투원 등록 — AI 타깃 획득이 이 등록소만 질의하면 World 전체 스캔이 사라진다.
	FCombatTargetRegistry::Get().Register(this);
}

void UCombatStateComponent::EndPlay()
{
	FCombatTargetRegistry::Get().Unregister(this);
	UActorComponent::EndPlay();
}

void UCombatStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bStaggered)
	{
		StaggerRemainingTime -= DeltaTime;
		if (StaggerRemainingTime <= 0.0f)
		{
			StopStagger();
		}
		return;
	}

	if (PoiseRecoveryPerSecond > 0.0f && CurrentPoise < MaxPoise)
	{
		float Rate = PoiseRecoveryPerSecond;
		if (bPostureRecoveryScalesWithHealth)
		{
			// 체력 100%→1.0배, 0%→0.4배. 체력이 낮을수록 체간 회복이 느려진다.
			Rate *= 0.4f + 0.6f * FMath::Clamp(GetHealthRatioSafe(), 0.0f, 1.0f);
		}
		CurrentPoise = FMath::Clamp(CurrentPoise + Rate * DeltaTime, 0.0f, MaxPoise);
	}
}

float UCombatStateComponent::GetHealthRatioSafe() const
{
	if (AActor* OwnerActor = GetOwner())
	{
		if (UHealthComponent* Health = OwnerActor->GetComponentByClass<UHealthComponent>())
		{
			return Health->GetHealthRatio();
		}
	}
	return 1.0f;
}

bool UCombatStateComponent::ApplyPoiseDamage(float PoiseDamage)
{
	if (PoiseDamage <= 0.0f || bSuperArmor || MaxPoise <= 0.0f)
	{
		return false;
	}

	CurrentPoise = FMath::Clamp(CurrentPoise - PoiseDamage, 0.0f, MaxPoise);
	if (CurrentPoise <= 0.0f)
	{
		// 자세 붕괴 신호 — ExecutionComponent 가 데스블로우 창을 연다. stagger 는 시각적 취약 표시로 유지.
		OnPostureBroken.Broadcast(this);
		StartStagger(DefaultStaggerDuration);
		return true;
	}
	return false;
}

void UCombatStateComponent::ResetPoise()
{
	CurrentPoise = MaxPoise;
}

void UCombatStateComponent::StartStagger(float Duration)
{
	if (Duration <= 0.0f)
	{
		ResetPoise();
		return;
	}
	bStaggered = true;
	StaggerRemainingTime = Duration;
	OnStaggerStarted.Broadcast(this, Duration);
}

void UCombatStateComponent::StopStagger()
{
	if (!bStaggered)
	{
		return;
	}
	bStaggered = false;
	StaggerRemainingTime = 0.0f;
	ResetPoise();
	OnStaggerEnded.Broadcast(this);
}

float UCombatStateComponent::GetPoiseRatio() const
{
	if (MaxPoise <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentPoise / MaxPoise, 0.0f, 1.0f);
}

void UCombatStateComponent::MarkAttacking(float WindowSeconds)
{
	if (WindowSeconds <= 0.0f)
	{
		return;
	}
	const float Until = GetOwnerGameTimeSeconds(this) + WindowSeconds;
	AttackThreatUntilSeconds = (std::max)(AttackThreatUntilSeconds, Until);
}

bool UCombatStateComponent::IsAttacking() const
{
	return GetAttackThreatRemaining() > 0.0f;
}

float UCombatStateComponent::GetAttackThreatRemaining() const
{
	const float Now = GetOwnerGameTimeSeconds(this);
	return (std::max)(0.0f, AttackThreatUntilSeconds - Now);
}

void UCombatStateComponent::MarkPerilous(EPerilousType InType, float Duration)
{
	if (InType == EPerilousType::None || Duration <= 0.0f)
	{
		return;
	}
	ActivePerilousType   = InType;
	PerilousUntilSeconds = GetOwnerGameTimeSeconds(this) + Duration;
	OnPerilousCue.Broadcast(this, InType);
}

bool UCombatStateComponent::IsPerilousActive() const
{
	return GetOwnerGameTimeSeconds(this) < PerilousUntilSeconds;
}

EPerilousType UCombatStateComponent::GetActivePerilousType() const
{
	return IsPerilousActive() ? ActivePerilousType : EPerilousType::None;
}

void UCombatStateComponent::OpenDeflectWindow(float Seconds)
{
	const float Now = GetOwnerGameTimeSeconds(this);
	const float Len = Seconds > 0.0f ? Seconds : DeflectWindowSeconds;
	DeflectOpenedAtSeconds = Now;
	DeflectUntilSeconds    = Now + Len;
	DeflectWindowLen       = Len;
}

bool UCombatStateComponent::IsDeflecting() const
{
	return GetOwnerGameTimeSeconds(this) < DeflectUntilSeconds;
}

EDeflectGrade UCombatStateComponent::ConsumeDeflect(AActor* Attacker)
{
	if (!IsDeflecting())
	{
		LastDeflectGrade = EDeflectGrade::None;
		return EDeflectGrade::None;
	}

	const float Now       = GetOwnerGameTimeSeconds(this);
	const float Remaining = DeflectUntilSeconds - Now;
	const float Frac      = DeflectWindowLen > 0.0f ? FMath::Clamp(Remaining / DeflectWindowLen, 0.0f, 1.0f) : 0.0f;

	// 윈도우 초반(많이 남음)에 받아넘길수록 정확 — Perfect/Good/Late.
	EDeflectGrade Grade;
	float ReflectScale;
	if (Frac >= 0.5f)
	{
		Grade        = EDeflectGrade::Perfect;
		ReflectScale = 1.0f;
	}
	else if (Frac >= 0.15f)
	{
		Grade        = EDeflectGrade::Good;
		ReflectScale = 0.6f;
	}
	else
	{
		Grade        = EDeflectGrade::Late;
		ReflectScale = 0.0f;
	}

	// 공격자에게 체간 반사 — 탄기가 공격자의 체간을 깎아 인살 창을 연다.
	// 위험공격 종류별 결과표(보고서 1군 #4)로 반사량을 보정한다: 찌르기 간파 성공은 큰 자세 손실,
	// 잡기는 탄기로 해결되지 않으므로 반사 0(점프/이탈로 대응해야 함).
	if (Attacker && ReflectScale > 0.0f && DeflectReflectPoise > 0.0f)
	{
		if (UCombatStateComponent* AttackerCombat = Attacker->GetComponentByClass<UCombatStateComponent>())
		{
			const FPerilousResolution Resolution = GetPerilousResolution(AttackerCombat->GetActivePerilousType());
			AttackerCombat->ApplyPoiseDamage(DeflectReflectPoise * ReflectScale * Resolution.AnswerReflectPoiseScale);
		}
	}

	DeflectUntilSeconds = Now; // 윈도우 닫기
	LastDeflectGrade    = Grade;
	OnDeflectResolved.Broadcast(this, Grade, Attacker);
	return Grade;
}

void UCombatStateComponent::OpenGuardWindow(float Seconds)
{
	const float Now = GetOwnerGameTimeSeconds(this);
	const float Len = Seconds > 0.0f ? Seconds : GuardWindowSeconds;
	GuardUntilSeconds = (std::max)(GuardUntilSeconds, Now + Len);
}

void UCombatStateComponent::CloseGuard()
{
	GuardUntilSeconds = GetOwnerGameTimeSeconds(this);
}

bool UCombatStateComponent::IsGuarding() const
{
	return GetOwnerGameTimeSeconds(this) < GuardUntilSeconds;
}

void UCombatStateComponent::OpenInvulnWindow(float Seconds)
{
	if (Seconds <= 0.0f)
	{
		return;
	}
	const float Now = GetOwnerGameTimeSeconds(this);
	InvulnUntilSeconds = (std::max)(InvulnUntilSeconds, Now + Seconds);
}

bool UCombatStateComponent::IsTimedInvuln() const
{
	return GetOwnerGameTimeSeconds(this) < InvulnUntilSeconds;
}

bool UCombatStateComponent::IsHostileTo(const UCombatStateComponent* Other) const
{
	if (!Other)
	{
		return false;
	}
	if (Team == ECombatTeam::Neutral || Other->Team == ECombatTeam::Neutral)
	{
		return false;
	}
	return Team != Other->Team;
}
