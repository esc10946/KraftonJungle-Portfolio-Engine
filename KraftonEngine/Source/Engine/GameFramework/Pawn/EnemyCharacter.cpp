#include "GameFramework/Pawn/EnemyCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/Montage/AnimMontage.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/EnemyAttackComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/HealthComponent.h"
#include "Component/Movement/CharacterMovementComponent.h"
#include "Component/Script/LuaScriptComponent.h"
#include "GameFramework/Controller/AIController.h"
#include "GameFramework/World.h"
#include "Object/Reflection/UClass.h"
#include "Math/MathUtils.h"
#include "Math/Rotator.h"

#include <algorithm>
#include <cmath>

namespace
{
	float NormalizeDeltaYaw(float Delta)
	{
		while (Delta > 180.0f) Delta -= 360.0f;
		while (Delta < -180.0f) Delta += 360.0f;
		return Delta;
	}

	float GetAttackExecutionDuration(const FEnemyAttackData& Attack)
	{
		const float FallbackEnd = Attack.bUseFallbackDamage ? Attack.FallbackDamageDelay + 0.1f : 0.0f;
		return (std::max)(0.05f, (std::max)(Attack.RecoveryTime, FallbackEnd));
	}
}

void AEnemyCharacter::BeginPlay()
{
	RebindEnemyComponents();
	Super::BeginPlay();
	if (bAutoPossessAI && !GetController())
	{
		SpawnDefaultAIController();
	}
}

void AEnemyCharacter::InitDefaultComponents(const FString& SkeletalMeshFileName)
{
	InitDefaultComponents(SkeletalMeshFileName, FString());
}

void AEnemyCharacter::InitDefaultComponents(const FString& SkeletalMeshFileName, const FString& ScriptFile)
{
	Super::InitDefaultComponents(SkeletalMeshFileName);
	AIBrainComponent = AddComponent<UEnemyAIBrainComponent>();
	AttackComponent = AddComponent<UEnemyAttackComponent>();
	LuaScriptComponent = AddComponent<ULuaScriptComponent>();
	if (LuaScriptComponent && !ScriptFile.empty())
	{
		LuaScriptComponent->SetScriptFile(ScriptFile);
	}
	bAutoInputWASD = false;
	bAutoInputMouseLook = false;
	if (!AIControllerClass)
	{
		AIControllerClass = AAIController::StaticClass();
	}
}

void AEnemyCharacter::PostDuplicate()
{
	Super::PostDuplicate();
	RebindEnemyComponents();
}

void AEnemyCharacter::OnPostLoad(FArchive& Ar)
{
	Super::OnPostLoad(Ar);
	RebindEnemyComponents();
}

void AEnemyCharacter::RebindEnemyComponents()
{
	AIBrainComponent = GetComponentByClass<UEnemyAIBrainComponent>();
	AttackComponent = GetComponentByClass<UEnemyAttackComponent>();
	LuaScriptComponent = GetComponentByClass<ULuaScriptComponent>();
	if (!AIControllerClass)
	{
		AIControllerClass = AAIController::StaticClass();
	}
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bUseBuiltInDecisionLogic)
	{
		RunBuiltInDecisionLogic(DeltaTime);
	}
	else
	{
		// Lua 두뇌가 의사결정을 가져간 경우에도, 진행 중인 공격 스윙(타격 윈도우·회복 종료)
		// 은 C++ 가 계속 실행해 준다 — "결정은 Lua, 실행은 C++".
		UpdateAttackExecution(DeltaTime);
	}
}

AAIController* AEnemyCharacter::SpawnDefaultAIController()
{
	if (AAIController* Existing = GetEnemyAIController())
	{
		return Existing;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	UClass* ControllerClass = AIControllerClass ? AIControllerClass : AAIController::StaticClass();
	AActor* Spawned = World->SpawnActorByClass(ControllerClass);
	AAIController* Controller = Cast<AAIController>(Spawned);
	if (!Controller)
	{
		return nullptr;
	}
	Controller->Possess(this);
	return Controller;
}

AAIController* AEnemyCharacter::GetEnemyAIController() const
{
	return GetAIController();
}

void AEnemyCharacter::MoveToTarget(float Scale)
{
	if (!bCanMove || !AIBrainComponent || !AIBrainComponent->HasValidTarget())
	{
		return;
	}
	const FVector Direction = AIBrainComponent->GetFlatDirectionToTarget();
	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction, Scale);
	}
}

void AEnemyCharacter::FaceTarget(float DeltaTime, float OverrideYawRate)
{
	if (!bCanRotate || !AIBrainComponent || !AIBrainComponent->HasValidTarget())
	{
		return;
	}
	const FVector Direction = AIBrainComponent->GetFlatDirectionToTarget();
	if (Direction.IsNearlyZero())
	{
		return;
	}
	FRotator Rotation = GetActorRotation();
	const float TargetYaw = atan2f(Direction.Y, Direction.X) * FMath::RadToDeg;
	const float Rate = OverrideYawRate > 0.0f ? OverrideYawRate : TurnSpeed;
	float DeltaYaw = NormalizeDeltaYaw(TargetYaw - Rotation.Yaw);
	const float MaxStep = Rate * DeltaTime;
	DeltaYaw = FMath::Clamp(DeltaYaw, -MaxStep, MaxStep);
	Rotation.Yaw += DeltaYaw;
	SetActorRotation(Rotation);
}

void AEnemyCharacter::StopEnemyMovement()
{
	if (AAIController* Controller = GetEnemyAIController())
	{
		Controller->StopMovement();
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->ClearInputVector();
		Movement->StopMovementImmediately();
	}
}

void AEnemyCharacter::StrafeAroundTarget(float Scale, bool bClockwise)
{
	if (!bCanMove || !AIBrainComponent || !AIBrainComponent->HasValidTarget())
	{
		return;
	}
	const FVector Direction = AIBrainComponent->GetFlatDirectionToTarget();
	if (Direction.IsNearlyZero())
	{
		return;
	}
	// XY 평면에서 타깃 방향을 90도 회전 — (x, y) -> (-y, x). 시계 반대면 부호 반전.
	FVector Perpendicular(-Direction.Y, Direction.X, 0.0f);
	if (!bClockwise)
	{
		Perpendicular = Perpendicular * -1.0f;
	}
	AddMovementInput(Perpendicular, Scale);
}

void AEnemyCharacter::RetreatFromTarget(float Scale)
{
	if (!bCanMove || !AIBrainComponent || !AIBrainComponent->HasValidTarget())
	{
		return;
	}
	const FVector Away = AIBrainComponent->GetFlatDirectionToTarget() * -1.0f;
	if (Away.IsNearlyZero())
	{
		return;
	}
	AddMovementInput(Away, Scale);
}

bool AEnemyCharacter::RequestMoveToTarget(float AcceptanceRadius, bool bUsePathfinding)
{
	if (!AIBrainComponent || !AIBrainComponent->HasValidTarget())
	{
		return false;
	}
	return RequestMoveToActor(AIBrainComponent->GetTarget(), AcceptanceRadius, bUsePathfinding);
}

bool AEnemyCharacter::RequestMoveToActor(AActor* Target, float AcceptanceRadius, bool bUsePathfinding)
{
	AAIController* Controller = GetEnemyAIController();
	if (!Controller)
	{
		Controller = SpawnDefaultAIController();
	}
	if (!Controller || !Target)
	{
		return false;
	}
	const float Radius = AcceptanceRadius > 0.0f ? AcceptanceRadius : (AIBrainComponent ? AIBrainComponent->AttackRange : 3.0f);
	return Controller->MoveToActor(Target, Radius, true, bUsePathfinding) != EPathFollowingRequestResult::Failed;
}

bool AEnemyCharacter::RequestMoveToLocation(const FVector& Location, float AcceptanceRadius, bool bUsePathfinding)
{
	AAIController* Controller = GetEnemyAIController();
	if (!Controller)
	{
		Controller = SpawnDefaultAIController();
	}
	if (!Controller)
	{
		return false;
	}
	const float Radius = AcceptanceRadius > 0.0f ? AcceptanceRadius : 3.0f;
	return Controller->MoveToLocation(Location, Radius, bUsePathfinding) != EPathFollowingRequestResult::Failed;
}

bool AEnemyCharacter::IsPathFollowing() const
{
	AAIController* Controller = GetEnemyAIController();
	return Controller && Controller->IsFollowingPath();
}

bool AEnemyCharacter::SelectAndCommitAttack(int32 Phase, FEnemyAttackData& OutAttack)
{
	OutAttack = FEnemyAttackData();
	if (!bCanAttack || !AIBrainComponent || !AttackComponent || !AIBrainComponent->HasValidTarget())
	{
		return false;
	}
	const float Distance = AIBrainComponent->GetDistanceToTarget();
	const float AbsAngle = fabsf(AIBrainComponent->GetAngleToTarget());
	OutAttack = AttackComponent->SelectAttackForStyle(
		Phase,
		Distance,
		AbsAngle,
		GetResolvedBehaviorStyle(),
		AIBrainComponent->GetStateTime(),
		GetCurrentHealthRatio());
	if (!OutAttack.AttackName.IsValid())
	{
		return false;
	}
	return AttackComponent->CommitAttackData(OutAttack);
}

bool AEnemyCharacter::PlayAttackMontage(const FEnemyAttackData& Attack)
{
	return StartAttackExecution(Attack);
}

bool AEnemyCharacter::StartAttackExecution(const FEnemyAttackData& Attack)
{
	if (!bCanAttack || !Attack.AttackName.IsValid())
	{
		return false;
	}
	if (!Attack.Montage && !Attack.bUseFallbackDamage)
	{
		return false;
	}

	CurrentAttack = Attack;
	CurrentAttackElapsed = 0.0f;
	bCurrentAttackActive = true;
	bFallbackDamageAttempted = false;
	CurrentAttackDamagedActors.clear();
	StopEnemyMovement();
	if (AIBrainComponent)
	{
		AIBrainComponent->SetState(FName("Attack"));
	}
	// 적 본인도 "공격 중" 으로 표시 — fallback 데미지(노티파이 없는) 공격까지 커버하고,
	// 플레이어/다른 AI 가 이 적의 공격을 폴링해 패링·회피할 수 있게 한다.
	if (UCombatStateComponent* Combat = GetCombatStateComponent())
	{
		Combat->MarkAttacking(GetAttackExecutionDuration(Attack));
	}

	const bool bMontageStarted = PlayCombatMontage(Attack.Montage);
	if (!bMontageStarted && !Attack.bUseFallbackDamage)
	{
		bCurrentAttackActive = false;
		CurrentAttack = FEnemyAttackData();
		CurrentAttackDamagedActors.clear();
		if (AIBrainComponent && !IsDead())
		{
			AIBrainComponent->SetState(FName("Guard"));
		}
		return false;
	}
	return true;
}

bool AEnemyCharacter::HasCurrentAttackDamagedActor(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}
	for (const TWeakObjectPtr<AActor>& Damaged : CurrentAttackDamagedActors)
	{
		if (Damaged.Get() == Target)
		{
			return true;
		}
	}
	return false;
}

bool AEnemyCharacter::MarkCurrentAttackDamagedActor(AActor* Target)
{
	if (!IsValid(Target) || HasCurrentAttackDamagedActor(Target))
	{
		return false;
	}
	CurrentAttackDamagedActors.push_back(Target);
	return true;
}

bool AEnemyCharacter::IsTargetHostileDamageReceiver(AActor* Target) const
{
	if (!IsValid(Target) || Target == this)
	{
		return false;
	}
	UHealthComponent* TargetHealth = Target->GetComponentByClass<UHealthComponent>();
	if (!TargetHealth || TargetHealth->IsDead())
	{
		return false;
	}
	UCombatStateComponent* OwnerCombat = GetCombatStateComponent();
	UCombatStateComponent* TargetCombat = Target->GetComponentByClass<UCombatStateComponent>();
	if (OwnerCombat && TargetCombat)
	{
		return OwnerCombat->IsHostileTo(TargetCombat);
	}
	return Target->HasTag(FName("Player")) || Target->HasTag(FName("HitTarget"));
}

bool AEnemyCharacter::IsTargetInsideCurrentAttackFallback(AActor* Target) const
{
	if (!HasCurrentAttack() || !IsValid(Target))
	{
		return false;
	}
	const float Radius = CurrentAttack.FallbackHitRadius > 0.0f ? CurrentAttack.FallbackHitRadius : CurrentAttack.MaxRange;
	const float Distance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
	if (Distance > Radius)
	{
		return false;
	}
	if (CurrentAttack.bRequiresTargetInFront && AIBrainComponent && AIBrainComponent->GetTarget() == Target)
	{
		return fabsf(AIBrainComponent->GetAngleToTarget()) <= CurrentAttack.MaxAbsAngle;
	}
	return true;
}

bool AEnemyCharacter::ApplyCurrentAttackDamageToActor(AActor* Target, const FVector& HitLocation)
{
	if (!HasCurrentAttack() || !IsTargetHostileDamageReceiver(Target) || HasCurrentAttackDamagedActor(Target))
	{
		return false;
	}
	UHealthComponent* TargetHealth = Target->GetComponentByClass<UHealthComponent>();
	if (!TargetHealth)
	{
		return false;
	}

	FCombatDamageSpec Spec;
	Spec.Damage = CurrentAttack.Damage;
	Spec.PoiseDamage = CurrentAttack.PoiseDamage;
	Spec.DamageCauser = this;
	Spec.InstigatorActor = this;
	Spec.HitLocation = HitLocation;
	FVector HitDirection = Target->GetActorLocation() - GetActorLocation();
	HitDirection.Z = 0.0f;
	Spec.HitDirection = HitDirection.IsNearlyZero() ? GetActorForward() : HitDirection.Normalized();

	const FCombatDamageReport Report = TargetHealth->ApplyDamageSpec(Spec);
	if (Report.Result == ECombatDamageResult::Damaged || Report.Result == ECombatDamageResult::Killed)
	{
		MarkCurrentAttackDamagedActor(Target);
		return true;
	}
	return false;
}

float AEnemyCharacter::GetCurrentHealthRatio() const
{
	return HealthComponent ? HealthComponent->GetHealthRatio() : 1.0f;
}

void AEnemyCharacter::UpdateAttackExecution(float DeltaTime)
{
	if (!bCurrentAttackActive)
	{
		return;
	}

	CurrentAttackElapsed += DeltaTime;
	FaceTarget(DeltaTime);

	if (CurrentAttack.bUseFallbackDamage && !bFallbackDamageAttempted && CurrentAttackElapsed >= CurrentAttack.FallbackDamageDelay)
	{
		bFallbackDamageAttempted = true;
		AActor* Target = AIBrainComponent ? AIBrainComponent->GetTarget() : nullptr;
		if (IsTargetInsideCurrentAttackFallback(Target))
		{
			ApplyCurrentAttackDamageToActor(Target, Target->GetActorLocation());
		}
	}

	const float Duration = GetAttackExecutionDuration(CurrentAttack);
	if (CurrentAttackElapsed >= Duration)
	{
		bCurrentAttackActive = false;
		bFallbackDamageAttempted = false;
		CurrentAttackElapsed = 0.0f;
		CurrentAttack = FEnemyAttackData();
		CurrentAttackDamagedActors.clear();
		if (AIBrainComponent && !IsDead())
		{
			AIBrainComponent->SetState(FName("Recover"));
		}
	}
}

void AEnemyCharacter::RunBuiltInDecisionLogic(float DeltaTime)
{
	if (!bUseBuiltInDecisionLogic)
	{
		return;
	}
	if (!AIBrainComponent || !AttackComponent)
	{
		RebindEnemyComponents();
	}
	if (!AIBrainComponent || !AttackComponent)
	{
		return;
	}
	if (IsDead())
	{
		AIBrainComponent->SetState(FName("Dead"));
		StopEnemyMovement();
		return;
	}
	if (UCombatStateComponent* CombatState = GetCombatStateComponent())
	{
		if (CombatState->IsStaggered())
		{
			AIBrainComponent->SetState(FName("Staggered"));
			StopEnemyMovement();
			return;
		}
	}

	UpdateAttackExecution(DeltaTime);
	if (bCurrentAttackActive)
	{
		return;
	}

	ThinkTimer -= DeltaTime;
	RepathTimer -= DeltaTime;
	if (ThinkTimer > 0.0f)
	{
		if (AIBrainComponent->HasValidTarget())
		{
			FaceTarget(DeltaTime);
		}
		return;
	}
	ThinkTimer = (std::max)(0.0f, ThinkInterval);

	if (!AIBrainComponent->HasValidTarget())
	{
		AIBrainComponent->AcquireDefaultTarget();
	}
	if (!AIBrainComponent->HasValidTarget())
	{
		AIBrainComponent->SetState(FName("Idle"));
		StopEnemyMovement();
		return;
	}

	FaceTarget(DeltaTime);
	const float Distance = AIBrainComponent->GetDistanceToTarget();
	const EEnemyAIBehaviorStyle Style = GetResolvedBehaviorStyle();

	FEnemyAttackData Attack;
	if (Style != EEnemyAIBehaviorStyle::Passive && bCanAttack)
	{
		Attack = AttackComponent->SelectAttackForStyle(
			GetCurrentAIPhase(),
			AIBrainComponent->GetDistanceToTarget(),
			fabsf(AIBrainComponent->GetAngleToTarget()),
			Style,
			AIBrainComponent->GetStateTime(),
			GetCurrentHealthRatio());
		if (Attack.AttackName.IsValid() && StartAttackExecution(Attack))
		{
			AttackComponent->CommitAttackData(Attack);
			return;
		}
	}

	if (Style == EEnemyAIBehaviorStyle::Defensive && Distance < DefensiveRetreatDistance)
	{
		AIBrainComponent->SetState(FName("Reposition"));
		StopEnemyMovement();
		const FVector Away = AIBrainComponent->GetFlatDirectionToTarget() * -1.0f;
		if (!Away.IsNearlyZero())
		{
			AddMovementInput(Away, DefensiveRetreatInputScale);
		}
		return;
	}

	const float Acceptance = (std::max)(0.1f, AIBrainComponent->AttackRange * 0.85f);
	if (Distance > AIBrainComponent->AttackRange)
	{
		AIBrainComponent->SetState(FName("Chase"));
		bool bMoveRequested = false;
		if (RepathTimer <= 0.0f)
		{
			bMoveRequested = AIBrainComponent->RequestMoveToTarget(Acceptance, true);
			RepathTimer = (std::max)(0.0f, RepathInterval);
		}
		if (!bMoveRequested && !AIBrainComponent->IsMoveActive())
		{
			MoveToTarget(1.0f);
		}
		return;
	}

	AIBrainComponent->SetState(FName("Guard"));
	StopEnemyMovement();
}

void AEnemyCharacter::HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor)
{
	Super::HandleDeath(Component, DamageCauser, InstigatorActor);
	bCanMove = false;
	bCanRotate = false;
	bCanAttack = false;
	bCurrentAttackActive = false;
	CurrentAttack = FEnemyAttackData();
	CurrentAttackDamagedActors.clear();
	StopEnemyMovement();
	if (AIBrainComponent)
	{
		AIBrainComponent->SetState(FName("Dead"));
	}
}
