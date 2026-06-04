#include "Component/AI/EnemyAIBrainComponent.h"

#include "Component/Combat/CombatStateComponent.h"
#include "GameFramework/AActor.h"
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
	}
	StateTime += DeltaTime;

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
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	UCombatStateComponent* OwnerCombat = OwnerActor ? OwnerActor->GetComponentByClass<UCombatStateComponent>() : nullptr;
	if (!World || !OwnerCombat)
	{
		return nullptr;
	}

	AActor* BestActor = nullptr;
	float BestDistanceSq = FLT_MAX;
	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const float Range = SearchRange > 0.0f ? SearchRange : DetectionRange;
	for (AActor* Candidate : World->GetActors())
	{
		if (!IsValid(Candidate) || Candidate == OwnerActor)
		{
			continue;
		}
		UCombatStateComponent* CandidateCombat = Candidate->GetComponentByClass<UCombatStateComponent>();
		if (!CandidateCombat || !OwnerCombat->IsHostileTo(CandidateCombat))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), OwnerLocation);
		if (Range > 0.0f && DistSq > Range * Range)
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

void UEnemyAIBrainComponent::SetState(const FName& NewState)
{
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
