#include "Component/Combat/CombatHitEventComponent.h"

#include "Component/Combat/HealthComponent.h"
#include "GameFramework/AActor.h"
#include "Profiling/Stats/Stats.h"

void UCombatHitEventComponent::BroadcastAttackHit(AActor* Target, UPrimitiveComponent* HitComponent, const FCombatDamageSpec& DamageSpec, FName HitEventName)
{
	SCOPE_STAT_CAT("CombatHitEvent.BroadcastAttackHit", "Combat");
	OnAttackHit.Broadcast(this, GetOwner(), Target, HitComponent, DamageSpec, HitEventName);
}

void UCombatHitEventComponent::BroadcastAttackParried(AActor* Defender, UPrimitiveComponent* HitComponent, const FCombatDamageSpec& DamageSpec, FName HitEventName)
{
	SCOPE_STAT_CAT("CombatHitEvent.BroadcastAttackParried", "Combat");
	OnAttackParried.Broadcast(this, GetOwner(), Defender, HitComponent, DamageSpec, HitEventName);
}

FCombatDamageReport UCombatHitEventComponent::ApplyDamageToTarget(AActor* Target, float Damage, float PoiseDamage, AActor* DamageCauser, AActor* InstigatorActor)
{
	FCombatDamageSpec DamageSpec;
	DamageSpec.Damage = Damage;
	DamageSpec.PoiseDamage = PoiseDamage;
	DamageSpec.DamageCauser = DamageCauser ? DamageCauser : GetOwner();
	DamageSpec.InstigatorActor = InstigatorActor ? InstigatorActor : GetOwner();

	return ApplyDamageSpecToTarget(Target, DamageSpec);
}

FCombatDamageReport UCombatHitEventComponent::ApplyDamageSpecToTarget(AActor* Target, const FCombatDamageSpec& DamageSpec)
{
	SCOPE_STAT_CAT("CombatHitEvent.ApplyDamageSpecToTarget", "Combat");
	FCombatDamageReport Report;
	Report.RequestedDamage = DamageSpec.Damage;

	if (!IsValid(Target))
	{
		Report.Result = ECombatDamageResult::Rejected;
		return Report;
	}

	UHealthComponent* HealthComponent = Target->GetComponentByClass<UHealthComponent>();
	if (!HealthComponent)
	{
		Report.Result = ECombatDamageResult::Rejected;
		return Report;
	}

	return HealthComponent->ApplyDamageSpec(DamageSpec);
}
