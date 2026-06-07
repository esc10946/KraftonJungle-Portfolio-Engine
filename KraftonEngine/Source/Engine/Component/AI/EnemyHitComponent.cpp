#include "Component/AI/EnemyHitComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/Montage/AnimMontage.h"
#include "Component/AI/EnemyAttackComponent.h"
#include "Component/Combat/CombatStateComponent.h"
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

	AActor* Owner = GetOwner();
	UCombatStateComponent* Combat = Owner ? Owner->GetComponentByClass<UCombatStateComponent>() : nullptr;

	// 슈퍼아머 보스가 자기 공격(선/활성/후딜)을 펼치는 중이면 일반 피격에 경직되지 않고 포즈를
	// 유지한다. 이러면 공세가 끊기지 않고(쿨다운도 리셋 안 함), 자세가 무너질 때만 stagger 된다.
	if (bPoiseThroughDuringOwnAttack && Combat && Combat->HasSuperArmor() && Combat->IsAttacking())
	{
		return;
	}

	if (bClearAttackCooldownsOnHit)
	{
		RestartAttackCooldowns();
	}

	// 가드 중 피격이면 가드 리액션(막기 흔들림)을, 아니면 방향별 피격 리액션을 재생한다.
	if (Combat && Combat->IsGuarding() && GuardHitMontage)
	{
		ACharacter* Character = Cast<ACharacter>(Owner);
		USkeletalMeshComponent* MeshComponent = Character ? Character->GetMesh() : nullptr;
		if (UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr)
		{
			if (bStopCurrentMontageBeforeHit)
			{
				AnimInstance->StopMontage(0.0f);
			}
			AnimInstance->PlayMontage(GuardHitMontage);
			return;
		}
	}

	PlayHitMontage(DamageCauser, InstigatorActor);
}

void UEnemyHitComponent::HandleDeath(UHealthComponent* /*Component*/, AActor* DamageCauser, AActor* InstigatorActor)
{
	if (bClearAttackCooldownsOnHit)
	{
		RestartAttackCooldowns();
	}
	PlayDeathMontage(DamageCauser, InstigatorActor);
}

void UEnemyHitComponent::RestartAttackCooldowns()
{
	AActor* OwnerActor = GetOwner();
	UEnemyAttackComponent* AttackComponent = OwnerActor ? OwnerActor->GetComponentByClass<UEnemyAttackComponent>() : nullptr;
	if (AttackComponent)
	{
		AttackComponent->RestartAttackCooldowns();
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
