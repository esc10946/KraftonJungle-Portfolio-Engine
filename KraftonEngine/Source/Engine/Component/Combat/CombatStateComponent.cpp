#include "Component/Combat/CombatStateComponent.h"

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
		CurrentPoise = FMath::Clamp(CurrentPoise + PoiseRecoveryPerSecond * DeltaTime, 0.0f, MaxPoise);
	}
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
