#include "GameFramework/Pawn/EnemyCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/Montage/AnimMontage.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/EnemyAttackComponent.h"
#include "Component/AI/AIBlackboardComponent.h"
#include "Component/AI/AIPerceptionComponent.h"
#include "Component/AI/UtilityReasonerComponent.h"
#include "Component/AI/AIDecisionTraceComponent.h"
#include "AI/SquadCoordinator.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/CombatMoveComponent.h"
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

#include "Core/Logging/Log.h"

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
	// 세키로식 전투 AI 코어 계층 — 책임 분리된 엔진 컴포넌트.
	Blackboard = AddComponent<UAIBlackboardComponent>();
	Perception = AddComponent<UAIPerceptionComponent>();
	Reasoner = AddComponent<UUtilityReasonerComponent>();
	DecisionTrace = AddComponent<UAIDecisionTraceComponent>();
	CombatMove = AddComponent<UCombatMoveComponent>();
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
	Blackboard = GetComponentByClass<UAIBlackboardComponent>();
	Perception = GetComponentByClass<UAIPerceptionComponent>();
	Reasoner = GetComponentByClass<UUtilityReasonerComponent>();
	DecisionTrace = GetComponentByClass<UAIDecisionTraceComponent>();
	CombatMove = GetComponentByClass<UCombatMoveComponent>();
	if (!AIControllerClass)
	{
		AIControllerClass = AAIController::StaticClass();
	}
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	LastTickDelta = DeltaTime;
	// 프레임 데이터 타임라인은 어느 결정 경로에서든(내장/Lua) 진행 — 위험표식·후딜 창을 만든다.
	if (UCombatMoveComponent* Move = CombatMove.Get())
	{
		Move->TickMove(DeltaTime);
	}
	if (bUseBuiltInDecisionLogic)
	{
		RunBuiltInDecisionLogic(DeltaTime);
	}
	else
	{
		// Lua 두뇌가 의사결정을 가져간 경우에도, 진행 중인 공격 스윙(타격 윈도우·회복 종료)
		// 은 C++ 가 계속 실행해 준다 — "결정은 Lua, 실행은 C++".
		// 전투 고정틱 시계를 누적해 Lua 가 Brain_ConsumeCombatStep 으로 결정을 게이트한다.
		// LOD 가 거리별로 스텝 주기를 조절(원거리 적은 저빈도) — Phase 4.
		UpdateAILOD();
		CombatClock.Accumulate(DeltaTime);
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
		Movement->StopHorizontalMovementImmediately();
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
	// 프레임 데이터 타임라인 시작 — startup/active/recovery + 위험표식.
	if (UCombatMoveComponent* Move = CombatMove.Get())
	{
		Move->BeginMove(Attack.PerilousType, Attack.StartupFrames, Attack.ActiveFrames, Attack.RecoveryFrames, Attack.PerilousCueFrame, Attack.bCanBeDeflected);
	}
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
		if (RepathTimer <= 0.0f || !AIBrainComponent->IsMoveActive())
		{
			bMoveRequested = AIBrainComponent->RequestMoveToTarget(Acceptance, true);
			RepathTimer = (std::max)(0.0f, RepathInterval);
		}
		if (!bMoveRequested && !AIBrainComponent->IsMoveActive())
		{
			StopEnemyMovement();
			UE_LOG("[EnemyAI] Chase path request failed. Enemy=%s Reason=%s", GetName().c_str(), AIBrainComponent->GetLastMoveFailureReason().c_str());
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
	if (UCombatMoveComponent* Move = CombatMove.Get())
	{
		Move->EndMove();
	}
	FSquadCoordinator::Get().ReleaseToken(this); // 죽으면 공격 토큰 반환 → 다른 적이 압박 이어감
	StopEnemyMovement();
	if (AIBrainComponent)
	{
		AIBrainComponent->SetState(FName("Dead"));
	}
}

// =============================================================================
//  Brain 동사 파사드 — Lua Blueprint 정책 ↔ C++ 코어 컴포넌트의 경계.
//  블루프린트는 이 무인자 동사들만 호출한다. 내부에서 Perception/Reasoner/
//  Blackboard/Brain 컴포넌트로 위임한다.
// =============================================================================
void AEnemyCharacter::Brain_Sense()
{
	if (UAIPerceptionComponent* P = Perception.Get())
	{
		P->UpdateSenses();
	}
}

bool AEnemyCharacter::Brain_AcquireTarget()
{
	UEnemyAIBrainComponent* Brain = AIBrainComponent.Get();
	if (!Brain)
	{
		return false;
	}
	if (!Brain->HasValidTarget())
	{
		Brain->AcquireDefaultTarget();
	}
	return Brain->HasValidTarget();
}

void AEnemyCharacter::Brain_FaceTarget()
{
	FaceTarget(LastTickDelta);
}

float AEnemyCharacter::Brain_GetDistance() const
{
	return AIBrainComponent.IsValid() ? AIBrainComponent->GetDistanceToTarget() : 9999.0f;
}

float AEnemyCharacter::Brain_GetAttackRange() const
{
	return AIBrainComponent.IsValid() ? AIBrainComponent->AttackRange : 3.0f;
}

bool AEnemyCharacter::Brain_ConsumeCombatStep()
{
	return CombatClock.ConsumeStep();
}

bool AEnemyCharacter::Brain_IsBusy() const
{
	if (IsDead() || HasCurrentAttack())
	{
		return true;
	}
	if (UCombatStateComponent* Combat = GetCombatStateComponent())
	{
		if (Combat->IsStaggered())
		{
			return true;
		}
	}
	return false;
}

bool AEnemyCharacter::Brain_SelectAttack()
{
	UUtilityReasonerComponent* R = Reasoner.Get();
	if (!R)
	{
		return false;
	}
	const FName Name = R->SelectBestAction();
	SelectedAttackName = Name;
	if (UAIDecisionTraceComponent* Trace = DecisionTrace.Get())
	{
		Trace->RecordDecision();
	}
	return Name.IsValid();
}

bool AEnemyCharacter::Brain_PlaySelectedAttack()
{
	if (!bCanAttack || !SelectedAttackName.IsValid() || !AttackComponent.IsValid())
	{
		return false;
	}
	const FEnemyAttackData Attack = AttackComponent->FindAttackByName(SelectedAttackName);
	if (!Attack.AttackName.IsValid())
	{
		return false;
	}
	if (!StartAttackExecution(Attack))
	{
		return false;
	}
	AttackComponent->CommitAttackData(Attack);
	if (UAIBlackboardComponent* BB = Blackboard.Get())
	{
		BB->PushRecentMove(Attack.AttackName);
	}
	return true;
}

void AEnemyCharacter::Brain_Chase()
{
	UEnemyAIBrainComponent* Brain = AIBrainComponent.Get();
	if (!Brain)
	{
		return;
	}
	Brain->SetState(FName("Chase"));
	const float Acceptance = (std::max)(0.1f, Brain->AttackRange * 0.85f);
	if (!Brain->RequestMoveToTarget(Acceptance, true) && !Brain->IsMoveActive())
	{
		StopEnemyMovement();
		UE_LOG("[EnemyAI] Brain_Chase path request failed. Enemy=%s Reason=%s", GetName().c_str(), Brain->GetLastMoveFailureReason().c_str());
	}
}

void AEnemyCharacter::Brain_Strafe()
{
	if (UEnemyAIBrainComponent* Brain = AIBrainComponent.Get())
	{
		Brain->SetState(FName("Strafe"));
	}
	StopEnemyMovement();
	StrafeAroundTarget(0.7f, bStrafeClockwise);
}

void AEnemyCharacter::Brain_Reposition()
{
	if (UEnemyAIBrainComponent* Brain = AIBrainComponent.Get())
	{
		Brain->SetState(FName("Reposition"));
	}
	StopEnemyMovement();
	// 공격 불가(쿨다운 등)일 때 간격을 유지하며 측면으로 빠진다. 방향은 가끔 뒤집어
	// 한쪽으로만 도는 것을 막는다.
	StrafeAroundTarget(0.6f, bStrafeClockwise);
	bStrafeClockwise = !bStrafeClockwise;
}

void AEnemyCharacter::Brain_Idle()
{
	if (UEnemyAIBrainComponent* Brain = AIBrainComponent.Get())
	{
		Brain->SetState(FName("Idle"));
	}
	FSquadCoordinator::Get().ReleaseToken(this); // 타깃 없음 → 토큰 반환
	StopEnemyMovement();
}

bool AEnemyCharacter::Brain_TargetThreatening() const
{
	const UEnemyAIBrainComponent* Brain = AIBrainComponent.Get();
	if (!Brain || !Brain->HasValidTarget())
	{
		return false;
	}
	AActor* Target = Brain->GetTarget();
	if (!Target)
	{
		return false;
	}
	UCombatStateComponent* TargetCombat = Target->GetComponentByClass<UCombatStateComponent>();
	if (!TargetCombat || !TargetCombat->IsAttacking())
	{
		return false;
	}
	return Brain->GetDistanceToTarget() <= Brain->AttackRange * 1.8f;
}

bool AEnemyCharacter::Brain_TargetInRecovery() const
{
	return Blackboard.IsValid() && Blackboard->GetBool(FName("TargetInRecovery"));
}

int32 AEnemyCharacter::Brain_GetTargetPerilous() const
{
	if (!Blackboard.IsValid())
	{
		return 0;
	}
	return static_cast<int32>(Blackboard->GetFloat(FName("TargetPerilous")) + 0.5f);
}

void AEnemyCharacter::Brain_OpenDeflect()
{
	if (UCombatStateComponent* Combat = GetCombatStateComponent())
	{
		Combat->OpenDeflectWindow(0.0f); // 0 → 컴포넌트 기본 윈도우 길이
	}
	if (UEnemyAIBrainComponent* Brain = AIBrainComponent.Get())
	{
		Brain->SetState(FName("Deflect"));
	}
	StopEnemyMovement();
}

bool AEnemyCharacter::Brain_AcquireAttackToken()
{
	UEnemyAIBrainComponent* Brain = AIBrainComponent.Get();
	UWorld* World = GetWorld();
	if (!Brain || !Brain->HasValidTarget() || !World)
	{
		return false;
	}
	AActor* Target = Brain->GetTarget();
	if (!Target)
	{
		return false;
	}
	const float Now  = World->GetGameTimeSeconds();
	const bool  bOk  = FSquadCoordinator::Get().TryAcquireToken(this, Target, Now, SquadTokenDuration, SquadMaxSimultaneousAttackers);
	if (UAIBlackboardComponent* BB = Blackboard.Get())
	{
		BB->SetBool(FName("HoldingToken"), bOk);
		BB->SetFloat(FName("ActiveAttackers"), static_cast<float>(FSquadCoordinator::Get().CountActiveAttackers(Target, Now)));
		BB->SetFloat(FName("SquadSlot"), static_cast<float>(FSquadCoordinator::Get().GetSlotIndex(this, Target, Now)));
	}
	return bOk;
}

void AEnemyCharacter::Brain_ReleaseAttackToken()
{
	FSquadCoordinator::Get().ReleaseToken(this);
	if (UAIBlackboardComponent* BB = Blackboard.Get())
	{
		BB->SetBool(FName("HoldingToken"), false);
	}
}

int32 AEnemyCharacter::Brain_GetSquadSlot() const
{
	const UEnemyAIBrainComponent* Brain = AIBrainComponent.Get();
	UWorld* World = GetWorld();
	if (!Brain || !Brain->HasValidTarget() || !World)
	{
		return -1;
	}
	return FSquadCoordinator::Get().GetSlotIndex(const_cast<AEnemyCharacter*>(this), Brain->GetTarget(), World->GetGameTimeSeconds());
}

void AEnemyCharacter::UpdateAILOD()
{
	float Distance = 9999.0f;
	if (AIBrainComponent.IsValid() && AIBrainComponent->HasValidTarget())
	{
		Distance = AIBrainComponent->GetDistanceToTarget();
	}

	int32 Lod;
	float StepSeconds;
	if (Distance <= LODNearDistance)
	{
		Lod = 0;
		StepSeconds = 1.0f / 60.0f; // 근접: 풀레이트 정밀 틱
	}
	else if (Distance <= LODFarDistance)
	{
		Lod = 1;
		StepSeconds = 1.0f / 30.0f;
	}
	else
	{
		Lod = 2;
		StepSeconds = 1.0f / 10.0f; // 원거리: 저빈도 think 로 CPU 예산 절감
	}

	CurrentLODLevel = Lod;
	CombatClock.StepSeconds = StepSeconds;
	if (UAIBlackboardComponent* BB = Blackboard.Get())
	{
		BB->SetFloat(FName("LOD"), static_cast<float>(Lod));
	}
}
