#include "Component/AI/EnemyHitComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/Montage/AnimMontage.h"
#include "Component/AI/EnemyAttackComponent.h"
#include "Component/AI/PhaseComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/HealthComponent.h"
#include "Component/Movement/CharacterMovementComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Pawn/Character.h"

#include <cmath>

namespace
{
	float SanitizeMontagePlayRate(float PlayRate)
	{
		return PlayRate > 0.0f ? PlayRate : 1.0f;
	}
}

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
	const EEnemyHitDirection Direction = ResolveHitDirection(DamageCauser, InstigatorActor);

	// 상체 레이어 모드: "UpperBody" 슬롯으로 재생 → 상체 본에만 블렌드되어 하체 로코모션/공격이
	// 안 끊긴다. 풀바디 StopMontage 도 하지 않는다.
	if (bUseUpperBodyHitLayer)
	{
		UAnimMontage* Montage = SelectMontage(HitMontages, Direction);
		ACharacter* Character = Cast<ACharacter>(GetOwner());
		USkeletalMeshComponent* MeshComponent = Character ? Character->GetMesh() : nullptr;
		UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
		if (Montage && AnimInstance)
		{
			AnimInstance->SetUpperBodyMaskRoot(UpperBodyMaskRootBone);
			AnimInstance->PlayMontage(Montage, FName::None,
				SanitizeMontagePlayRate(HitMontagePlayRate), -1.0f,
				UAnimInstance::UpperBodyMontageSlot);
			return true;
		}
		return false;
	}

	return PlayDirectionalMontage(HitMontages, Direction, bStopCurrentMontageBeforeHit, HitMontagePlayRate);
}

bool UEnemyHitComponent::PlayDeathMontage(AActor* DamageCauser, AActor* InstigatorActor)
{
	return PlayDirectionalMontage(
		DeathMontages,
		ResolveHitDirection(DamageCauser, InstigatorActor),
		bStopCurrentMontageBeforeDeath,
		DeathMontagePlayRate
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

	// 경직 페이즈 제한: 현재 페이즈가 HitReactMaxPhase 를 넘으면 피격 리액션을 완전히 생략한다
	// (하이퍼아머). 예: 보스 HitReactMaxPhase=1 → P1 에서만 경직, P2/P3 는 끊김 없음.
	{
		int32 CurrentPhase = 1;
		if (UPhaseComponent* Phase = Owner ? Owner->GetComponentByClass<UPhaseComponent>() : nullptr)
		{
			CurrentPhase = Phase->GetCurrentPhase();
		}
		if (CurrentPhase > HitReactMaxPhase)
		{
			return;
		}
	}

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

	// 가드 중 피격이면 가드 리액션(막기 흔들림)을 재생(이동 중이어도 가드는 제자리이므로 항상).
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
			AnimInstance->PlayMontage(GuardHitMontage, FName::None, SanitizeMontagePlayRate(HitMontagePlayRate));
			return;
		}
	}

	// 이동(걷기/달리기) 중이면 경직 몽타주를 생략한다 — 풀바디 피격 몽타주가 걷기를 덮어 슬라이딩처럼
	// 보이던 문제 수정(슈퍼아머 보스는 칩 피해를 걸으며 흘린다). 정지 상태에서만 경직.
	if (bSkipHitWhileMoving && Owner)
	{
		if (UCharacterMovementComponent* Move = Owner->GetComponentByClass<UCharacterMovementComponent>())
		{
			const FVector V = Move->GetVelocityValue();
			if (std::sqrt(V.X * V.X + V.Y * V.Y) > 0.3f)
			{
				return;
			}
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
	bool bStopCurrentMontage,
	float PlayRate)
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
	AnimInstance->PlayMontage(Montage, FName::None, SanitizeMontagePlayRate(PlayRate));
	return true;
}
