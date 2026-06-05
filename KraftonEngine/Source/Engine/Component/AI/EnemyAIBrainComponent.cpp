#include "Component/AI/EnemyAIBrainComponent.h"

#include "AI/CombatTargetRegistry.h"
#include "Component/Character/CharacterStateMachineComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Pawn/EnemyCharacter.h"
#include "GameFramework/Controller/AIController.h"
#include "GameFramework/World.h"
#include "Math/MathUtils.h"

#include <cfloat>
#include <cmath>

namespace
{
	float GetFlatAngleBetween(const FVector& Forward, const FVector& Direction)
	{
		FVector A = Forward;
		FVector B = Direction;
		A.Z = 0.0f;
		B.Z = 0.0f;
		if (A.IsNearlyZero() || B.IsNearlyZero())
		{
			return 0.0f;
		}
		A = A.Normalized();
		B = B.Normalized();
		const float Dot = FMath::Clamp(A.Dot(B), -1.0f, 1.0f);
		float Angle = acosf(Dot) * FMath::RadToDeg;
		const float CrossZ = A.X * B.Y - A.Y * B.X;
		if (CrossZ < 0.0f)
		{
			Angle = -Angle;
		}
		return Angle;
	}

	// FName HFSM 상태 ↔ 단일 권위 ECharacterState 매핑.
	ECharacterState MapFNameToState(const FName& StateName)
	{
		const FString N = StateName.ToString();
		if (N == "Chase" || N == "Pursue" || N == "Approach") return ECharacterState::Approach;
		if (N == "Strafe") return ECharacterState::Strafe;
		if (N == "Reposition" || N == "Retreat") return ECharacterState::Retreat;
		if (N == "Attack") return ECharacterState::Attack;
		if (N == "Deflect" || N == "Dodge" || N == "Parry" || N == "Defend") return ECharacterState::Defend;
		if (N == "Hit") return ECharacterState::Hit;
		if (N == "Staggered" || N == "Stagger") return ECharacterState::Staggered;
		if (N == "Dead") return ECharacterState::Dead;
		return ECharacterState::Idle; // Idle/Guard/Recover/unknown
	}

	FName MapStateToFName(ECharacterState State)
	{
		switch (State)
		{
		case ECharacterState::Approach:  return FName("Chase");
		case ECharacterState::Strafe:    return FName("Strafe");
		case ECharacterState::Retreat:   return FName("Reposition");
		case ECharacterState::Attack:    return FName("Attack");
		case ECharacterState::Defend:    return FName("Deflect");
		case ECharacterState::Hit:       return FName("Hit");
		case ECharacterState::Staggered: return FName("Staggered");
		case ECharacterState::Dead:      return FName("Dead");
		default:                         return FName("Idle");
		}
	}

	bool IsEventState(ECharacterState State)
	{
		return State == ECharacterState::Dead || State == ECharacterState::Staggered || State == ECharacterState::Hit;
	}
}

void UEnemyAIBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bHasInitializedState)
	{
		CurrentState = InitialState;
		PreviousState = FName::None;
		StateTime = 0.0f;
		bHasInitializedState = true;
		// 단일 권위 상태머신에도 초기 상태 반영.
		if (AActor* OwnerActor = GetOwner())
		{
			if (UCharacterStateMachineComponent* SM = OwnerActor->GetComponentByClass<UCharacterStateMachineComponent>())
			{
				SM->ForceState(MapFNameToState(InitialState));
			}
		}
	}
	StateTime += DeltaTime; // fallback 캐시 — GetStateTime 은 SM 이 있으면 그쪽을 우선.

	if (HasValidTarget() && LoseTargetRange > 0.0f && GetDistanceToTarget() > LoseTargetRange)
	{
		ClearTarget();
	}
}

void UEnemyAIBrainComponent::SetTarget(AActor* NewTarget)
{
	if (TargetActor.Get() == NewTarget)
	{
		return;
	}
	TargetActor = NewTarget;
	OnTargetChanged.Broadcast(this, NewTarget);
}

void UEnemyAIBrainComponent::ClearTarget()
{
	if (!TargetActor.IsValid())
	{
		TargetActor = nullptr;
		return;
	}
	TargetActor = nullptr;
	OnTargetChanged.Broadcast(this, nullptr);
}

bool UEnemyAIBrainComponent::HasValidTarget() const
{
	return TargetActor.IsValid();
}

AActor* UEnemyAIBrainComponent::AcquireTargetByTag(const FName& Tag)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	if (!World || !Tag.IsValid())
	{
		return nullptr;
	}

	AActor* BestActor = nullptr;
	float BestDistanceSq = FLT_MAX;
	const FVector OwnerLocation = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
	for (AActor* Candidate : World->GetActors())
	{
		if (!IsValid(Candidate) || Candidate == OwnerActor || !Candidate->HasTag(Tag))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), OwnerLocation);
		if (DetectionRange > 0.0f && DistSq > DetectionRange * DetectionRange)
		{
			continue;
		}
		if (DistSq < BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestActor = Candidate;
		}
	}
	SetTarget(BestActor);
	return BestActor;
}

AActor* UEnemyAIBrainComponent::AcquireNearestHostileTarget(float SearchRange)
{
	AActor* OwnerActor = GetOwner();
	UCombatStateComponent* OwnerCombat = OwnerActor ? OwnerActor->GetComponentByClass<UCombatStateComponent>() : nullptr;
	if (!OwnerActor || !OwnerCombat)
	{
		return nullptr;
	}
	const float Range = SearchRange > 0.0f ? SearchRange : DetectionRange;
	// World 전체 Actor 순회 대신 전투원 등록소만 질의 — 적 수가 늘어도 O(전투원 수).
	AActor* BestActor = FCombatTargetRegistry::Get().FindNearestHostile(OwnerCombat, OwnerActor->GetActorLocation(), Range);
	SetTarget(BestActor);
	return BestActor;
}

AActor* UEnemyAIBrainComponent::AcquireDefaultTarget()
{
	if (AActor* Hostile = AcquireNearestHostileTarget(DetectionRange))
	{
		return Hostile;
	}
	return AcquireTargetByTag(DefaultTargetTag);
}

float UEnemyAIBrainComponent::GetDistanceToTarget() const
{
	AActor* OwnerActor = GetOwner();
	AActor* Target = TargetActor.Get();
	if (!OwnerActor || !IsValid(Target))
	{
		return FLT_MAX;
	}
	return FVector::Distance(OwnerActor->GetActorLocation(), Target->GetActorLocation());
}

float UEnemyAIBrainComponent::GetFlatDistanceToTarget() const
{
	AActor* OwnerActor = GetOwner();
	AActor* Target = TargetActor.Get();
	if (!OwnerActor || !IsValid(Target))
	{
		return FLT_MAX;
	}
	FVector Delta = Target->GetActorLocation() - OwnerActor->GetActorLocation();
	Delta.Z = 0.0f;
	return Delta.Length();
}

float UEnemyAIBrainComponent::GetVerticalDeltaToTarget() const
{
	AActor* OwnerActor = GetOwner();
	AActor* Target = TargetActor.Get();
	if (!OwnerActor || !IsValid(Target))
	{
		return FLT_MAX;
	}
	return fabsf(Target->GetActorLocation().Z - OwnerActor->GetActorLocation().Z);
}

FVector UEnemyAIBrainComponent::GetDirectionToTarget() const
{
	AActor* OwnerActor = GetOwner();
	AActor* Target = TargetActor.Get();
	if (!OwnerActor || !IsValid(Target))
	{
		return FVector::ZeroVector;
	}
	const FVector Delta = Target->GetActorLocation() - OwnerActor->GetActorLocation();
	if (Delta.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}
	return Delta.Normalized();
}

FVector UEnemyAIBrainComponent::GetFlatDirectionToTarget() const
{
	FVector Direction = GetDirectionToTarget();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}
	return Direction.Normalized();
}

float UEnemyAIBrainComponent::GetAngleToTarget() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !HasValidTarget())
	{
		return 0.0f;
	}
	return GetFlatAngleBetween(OwnerActor->GetActorForward(), GetFlatDirectionToTarget());
}

bool UEnemyAIBrainComponent::IsTargetInRange(float Range) const
{
	return HasValidTarget() && GetDistanceToTarget() <= Range;
}

bool UEnemyAIBrainComponent::IsTargetInFront(float MaxAbsAngleDegrees) const
{
	return HasValidTarget() && fabsf(GetAngleToTarget()) <= MaxAbsAngleDegrees;
}

bool UEnemyAIBrainComponent::IsTargetBehind(float MinAbsAngleDegrees) const
{
	return HasValidTarget() && fabsf(GetAngleToTarget()) >= MinAbsAngleDegrees;
}

bool UEnemyAIBrainComponent::RequestMoveToTarget(float AcceptanceRadius, bool bUsePathfinding)
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetOwner());
	if (!Enemy || !HasValidTarget())
	{
		return false;
	}
	const float Radius = AcceptanceRadius > 0.0f ? AcceptanceRadius : AttackRange;
	return Enemy->RequestMoveToTarget(Radius, bUsePathfinding);
}

void UEnemyAIBrainComponent::StopMove()
{
	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetOwner()))
	{
		Enemy->StopEnemyMovement();
	}
}

bool UEnemyAIBrainComponent::IsMoveActive() const
{
	const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetOwner());
	return Enemy && Enemy->IsPathFollowing();
}

EPathFollowingStatus UEnemyAIBrainComponent::GetMoveStatus() const
{
	const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetOwner());
	AAIController* Controller = Enemy ? Enemy->GetEnemyAIController() : nullptr;
	return Controller ? Controller->GetMoveStatus() : EPathFollowingStatus::Idle;
}

EPathFollowingRequestResult UEnemyAIBrainComponent::GetLastMoveRequestResult() const
{
	const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetOwner());
	AAIController* Controller = Enemy ? Enemy->GetEnemyAIController() : nullptr;
	return Controller ? Controller->GetLastMoveRequestResult() : EPathFollowingRequestResult::Failed;
}

EPathFollowingResult UEnemyAIBrainComponent::GetLastMoveResult() const
{
	const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetOwner());
	AAIController* Controller = Enemy ? Enemy->GetEnemyAIController() : nullptr;
	return Controller ? Controller->GetLastMoveResult() : EPathFollowingResult::Invalid;
}

FString UEnemyAIBrainComponent::GetLastMoveFailureReason() const
{
	const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetOwner());
	AAIController* Controller = Enemy ? Enemy->GetEnemyAIController() : nullptr;
	return Controller ? Controller->GetLastMoveFailureReason() : "No AIController";
}

void UEnemyAIBrainComponent::SetState(const FName& NewState)
{
	// 단일 권위 상태머신이 있으면 그쪽으로 라우팅 — FName 상태는 enum 상태의 별칭이 된다.
	if (AActor* OwnerActor = GetOwner())
	{
		if (UCharacterStateMachineComponent* SM = OwnerActor->GetComponentByClass<UCharacterStateMachineComponent>())
		{
			const ECharacterState Target = MapFNameToState(NewState);
			const FName Before = MapStateToFName(SM->GetState());
			if (IsEventState(Target))
			{
				SM->ForceState(Target);
			}
			else
			{
				SM->RequestState(Target); // committed 상태(Attack 등)에선 거부될 수 있음 — 의도된 가드
			}
			const FName After = MapStateToFName(SM->GetState());
			bHasInitializedState = true;
			if (After != Before)
			{
				PreviousState = Before;
				CurrentState = After;
				StateTime = 0.0f;
				OnStateChanged.Broadcast(this, Before, After);
			}
			return;
		}
	}

	// fallback (상태머신 없음) — 기존 동작.
	if (CurrentState == NewState)
	{
		return;
	}
	const FName OldState = CurrentState;
	PreviousState = OldState;
	CurrentState = NewState;
	StateTime = 0.0f;
	bHasInitializedState = true;
	OnStateChanged.Broadcast(this, OldState, NewState);
}

FName UEnemyAIBrainComponent::GetState() const
{
	if (AActor* OwnerActor = GetOwner())
	{
		if (UCharacterStateMachineComponent* SM = OwnerActor->GetComponentByClass<UCharacterStateMachineComponent>())
		{
			return MapStateToFName(SM->GetState());
		}
	}
	return CurrentState;
}

float UEnemyAIBrainComponent::GetStateTime() const
{
	if (AActor* OwnerActor = GetOwner())
	{
		if (UCharacterStateMachineComponent* SM = OwnerActor->GetComponentByClass<UCharacterStateMachineComponent>())
		{
			return SM->GetTimeInState();
		}
	}
	return StateTime;
}
