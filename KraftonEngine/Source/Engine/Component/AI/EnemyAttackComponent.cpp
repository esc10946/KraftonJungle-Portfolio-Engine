#include "Component/AI/EnemyAttackComponent.h"

#include "Math/MathUtils.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>

void UEnemyAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	GlobalCooldownRemaining = (std::max)(0.0f, GlobalCooldownRemaining - DeltaTime);
	for (auto It = Cooldowns.begin(); It != Cooldowns.end(); )
	{
		It->RemainingTime = (std::max)(0.0f, It->RemainingTime - DeltaTime);
		if (It->RemainingTime <= 0.0f)
		{
			It = Cooldowns.erase(It);
			continue;
		}
		++It;
	}
}

int32 UEnemyAttackComponent::FindCooldownIndex(const FName& AttackName) const
{
	for (int32 Index = 0; Index < static_cast<int32>(Cooldowns.size()); ++Index)
	{
		if (Cooldowns[Index].AttackName == AttackName)
		{
			return Index;
		}
	}
	return -1;
}

bool UEnemyAttackComponent::IsAttackOnCooldown(const FName& AttackName) const
{
	return GetAttackCooldownRemaining(AttackName) > 0.0f;
}

float UEnemyAttackComponent::GetAttackCooldownRemaining(const FName& AttackName) const
{
	const int32 Index = FindCooldownIndex(AttackName);
	return Index >= 0 ? Cooldowns[Index].RemainingTime : 0.0f;
}

void UEnemyAttackComponent::SetAttackCooldown(const FName& AttackName, float Cooldown)
{
	if (!AttackName.IsValid() || Cooldown <= 0.0f)
	{
		return;
	}
	const int32 Index = FindCooldownIndex(AttackName);
	if (Index >= 0)
	{
		Cooldowns[Index].RemainingTime = (std::max)(Cooldowns[Index].RemainingTime, Cooldown);
		return;
	}
	FEnemyAttackCooldownEntry Entry;
	Entry.AttackName = AttackName;
	Entry.RemainingTime = Cooldown;
	Cooldowns.push_back(Entry);
}

void UEnemyAttackComponent::ClearAttackCooldowns()
{
	Cooldowns.clear();
	GlobalCooldownRemaining = 0.0f;
}

bool UEnemyAttackComponent::CanUseAttack(const FEnemyAttackData& Attack, int32 Phase, float Distance, float AbsAngle) const
{
	if (!Attack.AttackName.IsValid())
	{
		return false;
	}
	if (Phase < Attack.MinPhase || Phase > Attack.MaxPhase)
	{
		return false;
	}
	if (Distance < Attack.MinRange || Distance > Attack.MaxRange)
	{
		return false;
	}
	if (Attack.bRequiresTargetInFront && fabsf(AbsAngle) > Attack.MaxAbsAngle)
	{
		return false;
	}
	if (GlobalCooldownRemaining > 0.0f || IsAttackOnCooldown(Attack.AttackName))
	{
		return false;
	}
	return Attack.Weight > 0.0f;
}

float UEnemyAttackComponent::GetRecentRepeatScale(const FEnemyAttackData& Attack) const
{
	float Scale = 1.0f;
	for (const FName& Recent : RecentAttacks)
	{
		if (Recent == Attack.AttackName)
		{
			Scale *= FMath::Clamp(Attack.RepeatWeightScale, 0.0f, 1.0f);
		}
	}
	return Scale;
}


bool UEnemyAttackComponent::PassesComboGate(const FEnemyAttackData& Attack) const
{
	if (!Attack.bRequiresPreviousAttack)
	{
		return true;
	}
	return Attack.RequiredPreviousAttack.IsValid() && LastAttackName == Attack.RequiredPreviousAttack;
}

float UEnemyAttackComponent::GetStyleWeightScale(const FEnemyAttackData& Attack, EEnemyAIBehaviorStyle Style, float Distance, float AbsAngle, float StateTime, float HealthRatio) const
{
	float Scale = Attack.Priority > 0.0f ? Attack.Priority : 1.0f;
	const float RangeSpan = (std::max)(1.0f, Attack.MaxRange - Attack.MinRange);
	const float RangeT = FMath::Clamp((Distance - Attack.MinRange) / RangeSpan, 0.0f, 1.0f);
	const bool bVeryClose = RangeT <= 0.25f;
	const bool bFarInsideAttack = RangeT >= 0.65f;
	const bool bCleanFront = fabsf(AbsAngle) <= 35.0f;
	const bool bLowHealth = HealthRatio > 0.0f && HealthRatio <= 0.35f;
	const bool bHasRecentAttack = LastAttackName.IsValid();

	switch (Attack.Tactic)
	{
	case EEnemyAttackTactic::Opener:
		Scale *= (!bHasRecentAttack || StateTime <= 1.5f) ? 1.5f : 0.6f;
		break;
	case EEnemyAttackTactic::Pressure:
		Scale *= (Style == EEnemyAIBehaviorStyle::Aggressive || Style == EEnemyAIBehaviorStyle::Boss) ? 1.45f : 1.05f;
		break;
	case EEnemyAttackTactic::Combo:
		Scale *= bHasRecentAttack ? 1.6f : 0.35f;
		break;
	case EEnemyAttackTactic::GapCloser:
		Scale *= (bFarInsideAttack ? 1.5f : 0.55f);
		Scale *= (Style == EEnemyAIBehaviorStyle::Aggressive || Style == EEnemyAIBehaviorStyle::Boss) ? 1.35f : 0.9f;
		break;
	case EEnemyAttackTactic::Punish:
		Scale *= (bCleanFront && bVeryClose) ? 1.55f : 0.75f;
		break;
	case EEnemyAttackTactic::Retreat:
		Scale *= (bVeryClose || bLowHealth || Style == EEnemyAIBehaviorStyle::Defensive) ? 1.6f : 0.45f;
		break;
	case EEnemyAttackTactic::PhaseChange:
		Scale *= (Style == EEnemyAIBehaviorStyle::Boss) ? 1.35f : 0.7f;
		break;
	case EEnemyAttackTactic::Neutral:
	default:
		break;
	}

	switch (Style)
	{
	case EEnemyAIBehaviorStyle::Passive:
		Scale *= (Attack.Tactic == EEnemyAttackTactic::Retreat) ? 1.15f : 0.75f;
		break;
	case EEnemyAIBehaviorStyle::Aggressive:
		Scale *= (Attack.Tactic == EEnemyAttackTactic::Pressure || Attack.Tactic == EEnemyAttackTactic::GapCloser || Attack.bIsGapCloser) ? 1.25f : 1.0f;
		break;
	case EEnemyAIBehaviorStyle::Defensive:
		Scale *= (Attack.Tactic == EEnemyAttackTactic::Punish || Attack.Tactic == EEnemyAttackTactic::Retreat) ? 1.25f : 0.9f;
		break;
	case EEnemyAIBehaviorStyle::Boss:
		Scale *= (Attack.Tactic == EEnemyAttackTactic::PhaseChange || Attack.Tactic == EEnemyAttackTactic::Pressure || Attack.bIsGapCloser) ? 1.2f : 1.0f;
		break;
	case EEnemyAIBehaviorStyle::Balanced:
	default:
		break;
	}

	return (std::max)(0.0f, Scale);
}

FEnemyAttackData UEnemyAttackComponent::SelectAttack(int32 Phase, float Distance, float AbsAngle) const
{
	return SelectAttackForStyle(Phase, Distance, AbsAngle, EEnemyAIBehaviorStyle::Balanced, 0.0f, 1.0f);
}

FEnemyAttackData UEnemyAttackComponent::SelectAttackForStyle(int32 Phase, float Distance, float AbsAngle, EEnemyAIBehaviorStyle Style, float StateTime, float HealthRatio) const
{
	TArray<const FEnemyAttackData*> Candidates;
	float TotalWeight = 0.0f;
	for (const FEnemyAttackData& Attack : Attacks)
	{
		if (!CanUseAttack(Attack, Phase, Distance, AbsAngle) || !PassesComboGate(Attack))
		{
			continue;
		}
		const float EffectiveWeight = Attack.Weight
			* GetRecentRepeatScale(Attack)
			* GetStyleWeightScale(Attack, Style, Distance, AbsAngle, StateTime, HealthRatio);
		if (EffectiveWeight <= 0.0f)
		{
			continue;
		}
		Candidates.push_back(&Attack);
		TotalWeight += EffectiveWeight;
	}

	if (Candidates.empty() || TotalWeight <= 0.0f)
	{
		return FEnemyAttackData();
	}

	const float Random01 = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	float Pick = Random01 * TotalWeight;
	for (const FEnemyAttackData* Attack : Candidates)
	{
		const float EffectiveWeight = Attack->Weight
			* GetRecentRepeatScale(*Attack)
			* GetStyleWeightScale(*Attack, Style, Distance, AbsAngle, StateTime, HealthRatio);
		if (Pick <= EffectiveWeight)
		{
			return *Attack;
		}
		Pick -= EffectiveWeight;
	}
	return *Candidates.back();
}

bool UEnemyAttackComponent::CommitAttack(const FName& AttackName)
{
	const FEnemyAttackData Attack = FindAttackByName(AttackName);
	if (!Attack.AttackName.IsValid())
	{
		return false;
	}
	return CommitAttackData(Attack);
}

bool UEnemyAttackComponent::CommitAttackData(const FEnemyAttackData& Attack)
{
	if (!Attack.AttackName.IsValid())
	{
		return false;
	}
	SetAttackCooldown(Attack.AttackName, Attack.Cooldown);
	GlobalCooldownRemaining = (std::max)(GlobalCooldownRemaining, GlobalCooldown);
	LastAttackName = Attack.AttackName;
	RecentAttacks.insert(RecentAttacks.begin(), Attack.AttackName);
	while (static_cast<int32>(RecentAttacks.size()) > RecentAttackMemory)
	{
		RecentAttacks.pop_back();
	}
	OnAttackCommitted.Broadcast(this, Attack.AttackName);
	return true;
}

FEnemyAttackData UEnemyAttackComponent::FindAttackByName(const FName& AttackName) const
{
	for (const FEnemyAttackData& Attack : Attacks)
	{
		if (Attack.AttackName == AttackName)
		{
			return Attack;
		}
	}
	return FEnemyAttackData();
}
