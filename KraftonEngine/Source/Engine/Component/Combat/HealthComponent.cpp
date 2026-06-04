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
	if (OwnerActor)
	{
		if (UCombatStateComponent* CombatState = OwnerActor->GetComponentByClass<UCombatStateComponent>())
		{
			if (!CombatState->CanReceiveDamage())
			{
				Report.Result = ECombatDamageResult::Rejected;
				return Report;
			}
		}
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageSpec.Damage, 0.0f, MaxHealth);
	Report.NewHealth = CurrentHealth;
	Report.AppliedDamage = Report.PreviousHealth - Report.NewHealth;

	if (OwnerActor)
	{
		if (UCombatStateComponent* CombatState = OwnerActor->GetComponentByClass<UCombatStateComponent>())
		{
			CombatState->ApplyPoiseDamage(DamageSpec.PoiseDamage);
		}
	}

	OnDamaged.Broadcast(this, Report.AppliedDamage, CurrentHealth, DamageSpec.DamageCauser, DamageSpec.InstigatorActor);

	if (CurrentHealth <= 0.0f)
	{
		Report.Result = ECombatDamageResult::Killed;
		Report.bKilled = true;
		BroadcastDeathOnce(DamageSpec.DamageCauser, DamageSpec.InstigatorActor);
	}
	else
	{
		Report.Result = ECombatDamageResult::Damaged;
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
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	return CurrentHealth - Previous;
}

void UHealthComponent::SetHealth(float NewHealth)
{
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth > 0.0f ? MaxHealth : 1.0f);
	if (CurrentHealth > 0.0f)
	{
		bDead = false;
	}
	else
	{
		BroadcastDeathOnce(nullptr, nullptr);
	}
}

void UHealthComponent::Kill(AActor* DamageCauser, AActor* InstigatorActor)
{
	CurrentHealth = 0.0f;
	BroadcastDeathOnce(DamageCauser, InstigatorActor);
}

void UHealthComponent::ResetHealth()
{
	if (MaxHealth <= 0.0f)
	{
		MaxHealth = 1.0f;
	}
	CurrentHealth = MaxHealth;
	bDead = false;
}

float UHealthComponent::GetHealthRatio() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
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
