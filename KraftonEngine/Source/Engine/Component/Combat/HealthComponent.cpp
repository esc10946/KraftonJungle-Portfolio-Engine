#include "Component/Combat/HealthComponent.h"

#include "Component/Combat/CombatStateComponent.h"
#include "GameFramework/AActor.h"
#include "Math/MathUtils.h"

void UHealthComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	if (MaxHealth <= 0.0f)
	{
		MaxHealth = 1.0f;
	}
	if (bInitializeFullHealthOnBeginPlay)
	{
		CurrentHealth = MaxHealth;
		bDead = false;
	}
	else
	{
		CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
		bDead = CurrentHealth <= 0.0f;
	}
}

FCombatDamageReport UHealthComponent::ApplyDamage(float Damage, AActor* DamageCauser, AActor* InstigatorActor)
{
	FCombatDamageSpec Spec;
	Spec.Damage = Damage;
	Spec.DamageCauser = DamageCauser;
	Spec.InstigatorActor = InstigatorActor;
	return ApplyDamageSpec(Spec);
}

FCombatDamageReport UHealthComponent::ApplyDamageSpec(const FCombatDamageSpec& DamageSpec)
{
	FCombatDamageReport Report;
	Report.RequestedDamage = DamageSpec.Damage;
	Report.PreviousHealth = CurrentHealth;
	Report.NewHealth = CurrentHealth;

	if (DamageSpec.Damage <= 0.0f || bDead || bInvincible)
	{
		Report.Result = ECombatDamageResult::Rejected;
		return Report;
	}

	AActor* OwnerActor = GetOwner();
	float EffectiveDamage = DamageSpec.Damage;
	float EffectivePoiseDamage = DamageSpec.PoiseDamage;
	if (OwnerActor)
	{
		if (UCombatStateComponent* CombatState = OwnerActor->GetComponentByClass<UCombatStateComponent>())
		{
			if (!CombatState->CanReceiveDamage())
			{
				Report.Result = ECombatDamageResult::Rejected;
				return Report;
			}
			// 탄기 윈도우가 열려 있으면 받아넘김 — Perfect/Good 은 피해 무효 + 공격자 체간 반사,
			// Late 는 약화(chip)되어 통과. (윈도우 없으면 종전 동작 그대로 — 비파괴.)
			if (CombatState->IsDeflecting())
			{
				AActor* Attacker = DamageSpec.InstigatorActor ? DamageSpec.InstigatorActor : DamageSpec.DamageCauser;
				const EDeflectGrade Grade = CombatState->ConsumeDeflect(Attacker);
				if (Grade == EDeflectGrade::Perfect || Grade == EDeflectGrade::Good)
				{
					Report.Result = ECombatDamageResult::Deflected;
					Report.NewHealth = CurrentHealth;
					return Report;
				}
				if (Grade == EDeflectGrade::Late)
				{
					EffectiveDamage *= 0.5f;
					EffectivePoiseDamage *= 0.5f;
				}
			}
		}
	}

	SetHealthValue(CurrentHealth - EffectiveDamage, false);
	Report.NewHealth = CurrentHealth;
	Report.AppliedDamage = Report.PreviousHealth - Report.NewHealth;

	if (OwnerActor)
	{
		if (UCombatStateComponent* CombatState = OwnerActor->GetComponentByClass<UCombatStateComponent>())
		{
			CombatState->ApplyPoiseDamage(EffectivePoiseDamage);
		}
	}

	if (CurrentHealth <= 0.0f)
	{
		Report.Result = ECombatDamageResult::Killed;
		Report.bKilled = true;
	}
	else
	{
		Report.Result = ECombatDamageResult::Damaged;
	}

	OnDamageApplied.Broadcast(this, Report, DamageSpec);
	OnDamaged.Broadcast(this, Report.AppliedDamage, CurrentHealth, DamageSpec.DamageCauser, DamageSpec.InstigatorActor);

	if (Report.bKilled)
	{
		BroadcastDeathOnce(DamageSpec.DamageCauser, DamageSpec.InstigatorActor);
	}

	return Report;
}

float UHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || bDead)
	{
		return 0.0f;
	}
	const float Previous = CurrentHealth;
	SetHealthValue(CurrentHealth + Amount, false);
	return CurrentHealth - Previous;
}

void UHealthComponent::SetHealth(float NewHealth)
{
	SetHealthValue(NewHealth, true);
	if (CurrentHealth > 0.0f)
	{
		return;
	}
	BroadcastDeathOnce(nullptr, nullptr);
}

void UHealthComponent::Kill(AActor* DamageCauser, AActor* InstigatorActor)
{
	SetHealthValue(0.0f, false);
	BroadcastDeathOnce(DamageCauser, InstigatorActor);
}

void UHealthComponent::ResetHealth()
{
	if (MaxHealth <= 0.0f)
	{
		MaxHealth = 1.0f;
	}
	SetHealthValue(MaxHealth, true);
}

float UHealthComponent::GetHealthRatio() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
}

bool UHealthComponent::SetHealthValue(float NewHealth, bool bReviveIfPositive)
{
	const float PreviousHealth = CurrentHealth;
	const float ValidMaxHealth = MaxHealth > 0.0f ? MaxHealth : 1.0f;
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, ValidMaxHealth);

	if (bReviveIfPositive && CurrentHealth > 0.0f)
	{
		bDead = false;
	}

	if (CurrentHealth == PreviousHealth)
	{
		return false;
	}

	OnHealthChanged.Broadcast(this, PreviousHealth, CurrentHealth, ValidMaxHealth);
	return true;
}

void UHealthComponent::BroadcastDeathOnce(AActor* DamageCauser, AActor* InstigatorActor)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	CurrentHealth = 0.0f;
	OnDeath.Broadcast(this, DamageCauser, InstigatorActor);
}
