#include "Component/AI/EnemyHitComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/Montage/AnimMontage.h"
#include "Component/AI/EnemyAttackComponent.h"
#include "Component/Combat/HealthComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Pawn/Character.h"

#include <cmath>

void UEnemyHitComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	BindHealthEvents();
}

void UEnemyHitComponent::EndPlay()
{
	UnbindHealthEvents();
	UActorComponent::EndPlay();
}

void UEnemyHitComponent::BindHealthEvents()
{
	if (bBoundHealthEvents)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	UHealthComponent* HealthComponent = OwnerActor ? OwnerActor->GetComponentByClass<UHealthComponent>() : nullptr;
	if (!HealthComponent)
	{
		return;
	}

	BoundHealthComponent = HealthComponent;
	DamagedHandle = HealthComponent->OnDamaged.AddWeakUObject(this, &UEnemyHitComponent::HandleDamaged);
	DeathHandle = HealthComponent->OnDeath.AddWeakUObject(this, &UEnemyHitComponent::HandleDeath);
	bBoundHealthEvents = true;
}

void UEnemyHitComponent::UnbindHealthEvents()
{
	if (!bBoundHealthEvents)
	{
		return;
	}

	if (UHealthComponent* HealthComponent = BoundHealthComponent.Get())
	{
		if (DamagedHandle.IsValid())
		{
			HealthComponent->OnDamaged.Remove(DamagedHandle);
		}
		if (DeathHandle.IsValid())
		{
			HealthComponent->OnDeath.Remove(DeathHandle);
		}
	}

	DamagedHandle.Reset();
	DeathHandle.Reset();
	BoundHealthComponent.Reset();
	bBoundHealthEvents = false;
}

EEnemyHitDirection UEnemyHitComponent::ResolveHitDirection(AActor* DamageCauser, AActor* InstigatorActor) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return EEnemyHitDirection::Front;
	}

	AActor* SourceActor = DamageCauser ? DamageCauser : InstigatorActor;
	if (!SourceActor)
	{
		return EEnemyHitDirection::Front;
	}

	FVector SourceDirection = SourceActor->GetActorLocation() - OwnerActor->GetActorLocation();
	SourceDirection.Z = 0.0f;
	if (SourceDirection.IsNearlyZero())
	{
		return EEnemyHitDirection::Front;
	}
	SourceDirection = SourceDirection.Normalized();

	FVector Forward = OwnerActor->GetActorForward();
	Forward.Z = 0.0f;
	Forward = Forward.IsNearlyZero() ? FVector::ForwardVector : Forward.Normalized();

	FVector Right = OwnerActor->GetActorRight();
	Right.Z = 0.0f;
	Right = Right.IsNearlyZero() ? FVector::RightVector : Right.Normalized();

	const float ForwardDot = SourceDirection.Dot(Forward);
	const float RightDot = SourceDirection.Dot(Right);
	if (std::abs(ForwardDot) >= std::abs(RightDot))
	{
		return ForwardDot >= 0.0f ? EEnemyHitDirection::Front : EEnemyHitDirection::Back;
	}

	return RightDot >= 0.0f ? EEnemyHitDirection::Right : EEnemyHitDirection::Left;
}

bool UEnemyHitComponent::PlayHitMontage(AActor* DamageCauser, AActor* InstigatorActor)
{
	return PlayDirectionalMontage(
		HitMontages,
		ResolveHitDirection(DamageCauser, InstigatorActor),
		bStopCurrentMontageBeforeHit
	);
}

bool UEnemyHitComponent::PlayDeathMontage(AActor* DamageCauser, AActor* InstigatorActor)
{
	return PlayDirectionalMontage(
		DeathMontages,
		ResolveHitDirection(DamageCauser, InstigatorActor),
		bStopCurrentMontageBeforeDeath
	);
}

void UEnemyHitComponent::HandleDamaged(
	UHealthComponent* /*Component*/,
	float Damage,
	float NewHealth,
	AActor* DamageCauser,
	AActor* InstigatorActor)
{
	if (NewHealth <= 0.0f)
	{
		return;
	}
	if (Damage <= 0.0f && !bPlayHitWhenDamageIsZero)
	{
		return;
	}

	if (bClearAttackCooldownsOnHit)
	{
		ClearAttackCooldowns();
	}
	PlayHitMontage(DamageCauser, InstigatorActor);
}

void UEnemyHitComponent::HandleDeath(UHealthComponent* /*Component*/, AActor* DamageCauser, AActor* InstigatorActor)
{
	if (bClearAttackCooldownsOnHit)
	{
		ClearAttackCooldowns();
	}
	PlayDeathMontage(DamageCauser, InstigatorActor);
}

void UEnemyHitComponent::ClearAttackCooldowns()
{
	AActor* OwnerActor = GetOwner();
	UEnemyAttackComponent* AttackComponent = OwnerActor ? OwnerActor->GetComponentByClass<UEnemyAttackComponent>() : nullptr;
	if (AttackComponent)
	{
		AttackComponent->ClearAttackCooldowns();
	}
}

UAnimMontage* UEnemyHitComponent::SelectMontage(const FEnemyDirectionalMontages& Montages, EEnemyHitDirection Direction) const
{
	switch (Direction)
	{
	case EEnemyHitDirection::Front:
		return Montages.Front;
	case EEnemyHitDirection::Back:
		return Montages.Back;
	case EEnemyHitDirection::Left:
		return Montages.Left;
	case EEnemyHitDirection::Right:
		return Montages.Right;
	default:
		return Montages.Front;
	}
}

bool UEnemyHitComponent::PlayDirectionalMontage(
	const FEnemyDirectionalMontages& Montages,
	EEnemyHitDirection Direction,
	bool bStopCurrentMontage)
{
	UAnimMontage* Montage = SelectMontage(Montages, Direction);
	if (!Montage)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* MeshComponent = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	if (bStopCurrentMontage)
	{
		AnimInstance->StopMontage(0.0f);
	}
	AnimInstance->PlayMontage(Montage);
	return true;
}
