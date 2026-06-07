#include "GameFramework/Pawn/EnemyCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/Montage/AnimMontage.h"
#include "Animation/Notify/AnimNotifyState_ParryWindow.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/EnemyAttackComponent.h"
#include "Component/AI/EnemyHitComponent.h"
#include "Component/AI/AIBlackboardComponent.h"
#include "Component/AI/AIPerceptionComponent.h"
#include "Component/AI/AIDecisionTraceComponent.h"
#include "Component/AI/AwarenessComponent.h"
#include "AI/SquadCoordinator.h"
#include "AI/CombatTargetRegistry.h"
#include "Component/Character/CharacterStateMachineComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/CombatMoveComponent.h"
#include "Component/Combat/ExecutionComponent.h"
#include "Component/Combat/HealthComponent.h"
#include "Component/Movement/CharacterMovementComponent.h"
#include "Component/Shape/CapsuleComponent.h"
#include "Component/Script/LuaScriptComponent.h"
#include "Component/AI/BehaviorTreeComponent.h"
#include "AI/BehaviorTree/BehaviorTreeManager.h"
#include "Component/Script/LuaBlueprintComponent.h"
#include "LuaBlueprint/LuaBlueprintManager.h"
#include "LuaBlueprint/LuaBlueprintAsset.h"
#include "Core/Types/CollisionTypes.h"
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

	// 타격(데미지)이 들어가는 시점(초). 몽타주가 있으면 그 (배속 반영) 길이의 HitTimeFraction
	// 지점 — 실제 칼이 닿는 비주얼에 데미지를 맞춘다. 없으면 프레임데이터 활성 구간 중앙.
	float ComputeAttackHitTime(const FEnemyAttackData& Attack)
	{
		const float Rate = Attack.PlayRate > 0.0f ? Attack.PlayRate : 1.0f;
		if (Attack.Montage && Attack.Montage->GetPlayLength() > 0.0f)
		{
			const float MontageDur = Attack.Montage->GetPlayLength() / Rate;
			const float Frac = (std::max)(0.05f, (std::min)(0.95f, Attack.HitTimeFraction));
			return Frac * MontageDur;
		}
		return (static_cast<float>(Attack.StartupFrames) + 0.5f * static_cast<float>(Attack.ActiveFrames)) / 60.0f / Rate;
	}

	float GetAttackExecutionDuration(const FEnemyAttackData& Attack)
	{
		// 커밋 = 타격 시점 + 짧은 캔슬 버퍼. 긴 복귀(return) 꼬리는 커밋에 포함하지 않는다 —
		// 보스는 타격 직후 자유로워져 다음 공격을 블렌딩으로 잇는다(빠른 연속 교전, 턴제 느낌 제거).
		// 다음 공격의 짧은 블렌드인이 이전 몽타주의 복귀 구간을 자연스럽게 캔슬·크로스페이드한다.
		const float CancelBuffer = 0.16f;
		return (std::max)(0.12f, ComputeAttackHitTime(Attack) + CancelBuffer);
	}

	void StretchAttackFramesToDuration(FEnemyAttackData& Attack, float Duration)
	{
		const int32 RequiredTotalFrames = static_cast<int32>(std::ceil((std::max)(0.05f, Duration) * 60.0f));
		const int32 CurrentTotalFrames = Attack.StartupFrames + Attack.ActiveFrames + Attack.RecoveryFrames;
		if (CurrentTotalFrames < RequiredTotalFrames)
		{
			Attack.RecoveryFrames += RequiredTotalFrames - CurrentTotalFrames;
		}
	}

	// 액터의 캡슐 반경(없으면 기본값). 전투 거리/슬롯/분리를 "몸통 크기" 인식으로 만든다.
	float GetActorCapsuleRadius(AActor* Actor)
	{
		if (ACharacter* Character = Cast<ACharacter>(Actor))
		{
			if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				return Capsule->GetScaledCapsuleRadius();
			}
		}
		return 0.5f;
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
	HitComponent = AddComponent<UEnemyHitComponent>();
	// 세키로식 전투 AI 코어 계층 — 책임 분리된 엔진 컴포넌트.
	Blackboard = AddComponent<UAIBlackboardComponent>();
	Perception = AddComponent<UAIPerceptionComponent>();
	DecisionTrace = AddComponent<UAIDecisionTraceComponent>();
	CombatMove = AddComponent<UCombatMoveComponent>();
	Execution = AddComponent<UExecutionComponent>();
	Awareness = AddComponent<UAwarenessComponent>();
	BehaviorTree = AddComponent<UBehaviorTreeComponent>();
	LuaScriptComponent = AddComponent<ULuaScriptComponent>();
	BrainBlueprint = AddComponent<ULuaBlueprintComponent>();
	// 정책 드라이버 선택(Lua Blueprint 우선, 아니면 raw Lua). ScriptFile 이 명시되면 그것을 강제.
	ConfigureBrainDriver(ScriptFile);
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
	if (!AIBrainComponent.IsValid())
	{
		AIBrainComponent = AddComponent<UEnemyAIBrainComponent>();
	}
	AttackComponent = GetComponentByClass<UEnemyAttackComponent>();
	if (!AttackComponent.IsValid())
	{
		AttackComponent = AddComponent<UEnemyAttackComponent>();
	}
	HitComponent = GetComponentByClass<UEnemyHitComponent>();
	if (!HitComponent.IsValid())
	{
		HitComponent = AddComponent<UEnemyHitComponent>();
	}
	LuaScriptComponent = GetComponentByClass<ULuaScriptComponent>();
	if (!LuaScriptComponent.IsValid())
	{
		LuaScriptComponent = AddComponent<ULuaScriptComponent>();
	}
	BrainBlueprint = GetComponentByClass<ULuaBlueprintComponent>();
	if (!BrainBlueprint.IsValid())
	{
		BrainBlueprint = AddComponent<ULuaBlueprintComponent>();
	}
	ConfigureBrainDriver(FString());
	Blackboard = GetComponentByClass<UAIBlackboardComponent>();
	if (!Blackboard.IsValid())
	{
		Blackboard = AddComponent<UAIBlackboardComponent>();
	}
	Perception = GetComponentByClass<UAIPerceptionComponent>();
	if (!Perception.IsValid())
	{
		Perception = AddComponent<UAIPerceptionComponent>();
	}
	DecisionTrace = GetComponentByClass<UAIDecisionTraceComponent>();
	if (!DecisionTrace.IsValid())
	{
		DecisionTrace = AddComponent<UAIDecisionTraceComponent>();
	}
	CombatMove = GetComponentByClass<UCombatMoveComponent>();
	if (!CombatMove.IsValid())
	{
		CombatMove = AddComponent<UCombatMoveComponent>();
	}
	Execution = GetComponentByClass<UExecutionComponent>();
	if (!Execution.IsValid())
	{
		Execution = AddComponent<UExecutionComponent>();
	}
	Awareness = GetComponentByClass<UAwarenessComponent>();
	if (!Awareness.IsValid())
	{
		Awareness = AddComponent<UAwarenessComponent>();
	}
	BehaviorTree = GetComponentByClass<UBehaviorTreeComponent>();
	if (!BehaviorTree.IsValid())
	{
		BehaviorTree = AddComponent<UBehaviorTreeComponent>();
	}
	if (!AIControllerClass)
	{
		AIControllerClass = AAIController::StaticClass();
	}
	ConfigureBrainDriver(FString());
}

void AEnemyCharacter::ConfigureBrainDriver(const FString& ExplicitScriptOverride)
{
	ULuaScriptComponent*    Script = LuaScriptComponent.Get();
	ULuaBlueprintComponent* Bp     = BrainBlueprint.Get();
	UBehaviorTreeComponent* BT     = BehaviorTree.Get();

	// 1) 명시적 raw-lua 오버라이드(특수 적/테스트)는 최우선.
	if (!ExplicitScriptOverride.empty())
	{
		if (BT)     BT->SetBehaviorTreePath(FString());
		if (Bp)     Bp->SetBlueprintPath(FString());
		if (Script) Script->SetScriptFile(ExplicitScriptOverride);
		return;
	}

	// 2) 로드 가능한 Behavior Tree 가 지정됐으면 그것으로 두뇌를 구동(최우선 데이터 드라이버).
	if (BT && !BrainBehaviorTreeFile.empty())
	{
		if (FBehaviorTreeManager::Get().Load(BrainBehaviorTreeFile))
		{
			if (Script) Script->SetScriptFile(FString()); // 다른 두뇌 끄기(중복 구동 방지)
			if (Bp)     Bp->SetBlueprintPath(FString());
			BT->SetBehaviorTreePath(BrainBehaviorTreeFile);
			return;
		}
		UE_LOG("EnemyCharacter Brain Behavior Tree not loadable, falling back: %s", BrainBehaviorTreeFile.c_str());
	}
	if (BT) BT->SetBehaviorTreePath(FString()); // BT 미사용 — inert 로

	// 3) 구동 가능한 Lua Blueprint 가 지정됐으면 그것으로 두뇌를 구동(정책=Lua Blueprint).
	bool bUseBlueprint = false;
	if (!BrainBlueprintFile.empty())
	{
		ULuaBlueprintAsset* Asset = FLuaBlueprintManager::Get().Load(BrainBlueprintFile);
		bUseBlueprint = Asset && Asset->HasRunnableLuaSource();
		if (!bUseBlueprint)
		{
			UE_LOG("EnemyCharacter Brain Blueprint not runnable, falling back to lua: %s", BrainBlueprintFile.c_str());
		}
	}

	if (bUseBlueprint)
	{
		if (Script) Script->SetScriptFile(FString());      // raw-lua 두뇌 끄기(중복 구동 방지)
		if (Bp)     Bp->SetBlueprintPath(BrainBlueprintFile);
	}
	else
	{
		if (Bp) Bp->SetBlueprintPath(FString());
		if (Script && Script->GetScriptFile().empty() && !BrainScriptFile.empty())
		{
			Script->SetScriptFile(BrainScriptFile);
		}
	}
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	LastTickDelta = DeltaTime;

	UpdateAILOD();
	if (UCombatMoveComponent* Move = CombatMove.Get())
	{
		Move->TickMove(DeltaTime);
	}
	ApplySeparationSteering();

	// 의사결정은 Lua Blueprint가 전담한다. C++은 센서/이동/공격 실행 타임라인만 진행한다.
	CombatClock.Accumulate(DeltaTime);
	UpdateAttackExecution(DeltaTime);
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
	StopPathFollowingOnly();
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->ClearInputVector();
		Movement->StopHorizontalMovementImmediately();
	}
}

void AEnemyCharacter::StopPathFollowingOnly()
{
	if (AAIController* Controller = GetEnemyAIController())
	{
		Controller->StopMovement();
	}
}

bool AEnemyCharacter::SetRuntimeState(const FName& StateName, bool bForce)
{
	UCharacterStateMachineComponent* SM = GetStateMachine();
	if (!SM || !StateName.IsValid())
	{
		return false;
	}
	if (bForce)
	{
		SM->ForceState(StateName);
		return true;
	}
	return SM->RequestState(StateName);
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
	const float Radius = AcceptanceRadius > 0.0f ? AcceptanceRadius : DefaultAttackRange;
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

	FEnemyAttackData ExecutingAttack = Attack;
	StretchAttackFramesToDuration(ExecutingAttack, GetAttackExecutionDuration(Attack));

	CurrentAttack = ExecutingAttack;
	CurrentAttackElapsed = 0.0f;
	bCurrentAttackActive = true;
	// 타격 시점(비주얼 스윙에 맞춤) — 이 시점부터 짧은 윈도우 동안 데미지가 들어간다.
	CurrentAttackHitTime = ComputeAttackHitTime(CurrentAttack);
	bCurrentAttackHitApplied = false;
	CurrentAttackDamagedActors.clear();
	// 공격 문법 분기용 결과 캐시 리셋(보고서 1군 #3).
	LastAttackResult = ECombatDamageResult::None;
	bLastAttackConnected = false;
	// 몽타주 없는 공격이거나 bDamageFromFrameData 면 timed 폴백 경로로 데미지를 준다
	// (UpdateAttackExecution 에서 active 프레임에 적용). 후자는 비주얼 몽타주는 재생하되
	// 소스 시퀀스에 AttackHitWindow 노티파이가 없어도 프레임데이터 타임라인이 타격을 담당한다.
	bCurrentAttackMontageless = (CurrentAttack.Montage == nullptr) || CurrentAttack.bDamageFromFrameData;
	// 로코모션 구동이 "내 공격 몽타주 꼬리"는 이동 시 걷기로 덮어쓰되, 리액션(피격/가드/회피)
	// 몽타주에는 양보하도록 구분하기 위해 현재 공격 몽타주를 기록.
	CurrentAttackMontage = CurrentAttack.Montage;
	// 프레임 데이터 타임라인 시작 — startup/active/recovery + 위험표식.
	if (UCombatMoveComponent* Move = CombatMove.Get())
	{
		Move->BeginMove(CurrentAttack.PerilousType, CurrentAttack.StartupFrames, CurrentAttack.ActiveFrames, CurrentAttack.RecoveryFrames, CurrentAttack.PerilousCueFrame, CurrentAttack.bCanBeDeflected);
	}
	StopEnemyMovement();
	// 갭클로저는 "이동형 공격" — 상태머신에 전진 대시를 걸어 root motion 이 없어도 거리를 좁힌다.
	if (CurrentAttack.bIsGapCloser && GapCloserDashScale > 0.0f)
	{
		const float DashSeconds = (std::max)(0.15f, static_cast<float>(CurrentAttack.StartupFrames + CurrentAttack.ActiveFrames) / 60.0f);
		if (UCharacterStateMachineComponent* SM = GetStateMachine())
		{
			SM->BeginDash(DashSeconds, GapCloserDashScale);
		}
	}
	SetRuntimeState(FName("Attack"));
	// 적 본인도 "공격 중" 으로 표시해 플레이어/다른 AI가 패링·회피 판단에 사용할 수 있게 한다.
	if (UCombatStateComponent* Combat = GetCombatStateComponent())
	{
		Combat->MarkAttacking(GetAttackExecutionDuration(CurrentAttack));
	}

	// 몽타주가 없으면(에셋 미제작) 재생을 건너뛰고 timed 공격으로 진행한다.
	// 있으면 공격별 배속으로 재생하고, 콤보가 꼬리에서 이어지도록 짧은 블렌드인으로 크로스페이드한다.
	const bool bMontageStarted = CurrentAttack.Montage
		? PlayCombatMontageRated(CurrentAttack.Montage, CurrentAttack.PlayRate, 0.18f)
		: true;
	if (!bMontageStarted)
	{
		bCurrentAttackActive = false;
		bCurrentAttackMontageless = false;
		CurrentAttack = FEnemyAttackData();
		CurrentAttackDamagedActors.clear();
		if (!IsDead())
		{
			SetRuntimeState(FName("Guard"), true);
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

bool AEnemyCharacter::ApplyCurrentAttackDamageToActor(AActor* Target, const FVector& HitLocation)
{
	if (!HasCurrentAttack() || !IsTargetHostileDamageReceiver(Target) || HasCurrentAttackDamagedActor(Target))
	{
		return false;
	}

	// 플레이어 패링 — 이 공격은 montageless(프레임데이터) 경로라 AttackHitWindow 노티파이를
	// 타지 않는다. 따라서 여기서 타깃의 패링 윈도우를 직접 확인해 노티파이 경로와 동일하게
	// 처리한다: 피해 무효 + 패링 보고 + 공격자(보스) 자세 압박 + Deflected 결과 캐시(브레인이
	// 후속 행동을 가드/재배치로 바꾼다). 보스는 패링이 없고 가드만 하지만, 플레이어의 패링은
	// 반드시 보스 공격에 통해야 세키로 루프가 성립한다.
	if (USkeletalMeshComponent* TargetMesh = Target->GetComponentByClass<USkeletalMeshComponent>())
	{
		UAnimInstance* TargetAnim = TargetMesh->GetAnimInstance();
		if (TargetAnim && UAnimNotifyState_ParryWindow::IsParryWindowActive(TargetAnim))
		{
			UAnimNotifyState_ParryWindow::ReportSuccessfulParry(TargetAnim);
			MarkCurrentAttackDamagedActor(Target); // 이 공격으로 같은 타깃을 다시 처리하지 않음
			LastAttackResult = ECombatDamageResult::Deflected;
			bLastAttackConnected = false;
			if (UCombatStateComponent* SelfCombat = GetCombatStateComponent())
			{
				// 패링 성공 시 보스 자세를 압박한다(슈퍼아머면 흡수). 위험공격 종류별 반사 배수 반영.
				const FPerilousResolution Res = GetPerilousResolution(CurrentAttack.PerilousType);
				SelfCombat->ApplyPoiseDamage(SelfCombat->DeflectReflectPoise * Res.AnswerReflectPoiseScale);
			}
			return false;
		}
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
	// 공격 문법 분기용 결과 캐시: 탄기당함/적중을 Lua 가 읽어 후속을 바꾼다.
	LastAttackResult = Report.Result;
	if (Report.Result == ECombatDamageResult::Damaged || Report.Result == ECombatDamageResult::Killed)
	{
		bLastAttackConnected = true;
		MarkCurrentAttackDamagedActor(Target);
		return true;
	}
	return false;
}

float AEnemyCharacter::GetCurrentHealthRatio() const
{
	return HealthComponent ? HealthComponent->GetHealthRatio() : 1.0f;
}

bool AEnemyCharacter::AcquireAttackTokenForDuration(float DesiredDuration)
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
	const float Now = World->GetGameTimeSeconds();
	const float TokenDuration = (std::max)(SquadTokenDuration, DesiredDuration);
	const bool bOk = FSquadCoordinator::Get().TryAcquireToken(this, Target, Now, TokenDuration, SquadMaxSimultaneousAttackers);
	if (UAIBlackboardComponent* BB = Blackboard.Get())
	{
		BB->SetBool(FName("HoldingToken"), bOk);
		BB->SetFloat(FName("ActiveAttackers"), static_cast<float>(FSquadCoordinator::Get().CountActiveAttackers(Target, Now)));
		BB->SetFloat(FName("SquadSlot"), static_cast<float>(FSquadCoordinator::Get().GetSlotIndex(this, Target, Now)));
	}
	return bOk;
}

void AEnemyCharacter::UpdateAttackExecution(float DeltaTime)
{
	if (!bCurrentAttackActive)
	{
		return;
	}

	CurrentAttackElapsed += DeltaTime;
	FaceTarget(DeltaTime);

	// 프레임데이터 기반 타격: 타격 시점(CurrentAttackHitTime, 비주얼 스윙에 맞춤)부터 짧은
	// 윈도우 동안 사정권 내 타깃에 1회 데미지를 적용한다. CombatMove 의 active 프레임이 아니라
	// 몽타주 진행 비율로 정렬해, "칼이 닿기도 전에 데미지가 들어가는" 어긋남을 없앤다.
	if (bCurrentAttackMontageless && !bCurrentAttackHitApplied
		&& CurrentAttackElapsed >= CurrentAttackHitTime
		&& CurrentAttackElapsed <= CurrentAttackHitTime + 0.12f)
	{
		UEnemyAIBrainComponent* Brain = AIBrainComponent.Get();
		if (Brain && Brain->HasValidTarget())
		{
			if (AActor* Tgt = Brain->GetTarget())
			{
				const float Reach = (CurrentAttack.MaxRange > 0.0f ? CurrentAttack.MaxRange : DefaultAttackRange) + GetActorCapsuleRadius(Tgt) + 0.3f;
				if (Brain->GetFlatDistanceToTarget() <= Reach)
				{
					ApplyCurrentAttackDamageToActor(Tgt, Tgt->GetActorLocation());
					bCurrentAttackHitApplied = true;
				}
			}
		}
	}

	const float Duration = GetAttackExecutionDuration(CurrentAttack);
	if (CurrentAttackElapsed >= Duration)
	{
		Brain_ReleaseAttackToken();
		bCurrentAttackActive = false;
		bCurrentAttackMontageless = false;
		CurrentAttackElapsed = 0.0f;
		CurrentAttack = FEnemyAttackData();
		CurrentAttackDamagedActors.clear();
		if (!IsDead())
		{
			SetRuntimeState(FName("Recover"), true);
		}
	}
}

void AEnemyCharacter::HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor)
{
	Super::HandleDeath(Component, DamageCauser, InstigatorActor);
	bCanMove = false;
	bCanRotate = false;
	bCanAttack = false;
	bCurrentAttackActive = false;
	bCurrentAttackMontageless = false;
	CurrentAttack = FEnemyAttackData();
	CurrentAttackDamagedActors.clear();
	if (UCombatMoveComponent* Move = CombatMove.Get())
	{
		Move->EndMove();
	}
	FSquadCoordinator::Get().ReleaseToken(this);   // 죽으면 공격 토큰 반환 → 다른 적이 압박 이어감
	FSquadCoordinator::Get().ReleaseEngager(this); // 링 슬롯도 반환 → 남은 적이 재배치
	StopEnemyMovement();
	SetRuntimeState(FName("Dead"), true);
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
	// 단일 권위 상태머신에 이동 타깃 공급 — Strafe/Retreat/조준이 이 타깃을 쓴다.
	if (UCharacterStateMachineComponent* SM = GetStateMachine())
	{
		SM->SetMovementTarget(AIBrainComponent.IsValid() ? AIBrainComponent->GetTarget() : nullptr);
	}

	// 다수 적 슬롯팅: 타깃 교전 등록 → 슬롯/인원 캐시(겹침 없는 링 배치용).
	if (AIBrainComponent.IsValid() && AIBrainComponent->HasValidTarget())
	{
		AActor* Target = AIBrainComponent->GetTarget();
		UWorld* World  = GetWorld();
		if (Target && World)
		{
			const float Now    = World->GetGameTimeSeconds();
			CachedSquadSlot    = FSquadCoordinator::Get().RegisterEngager(this, Target, Now, SquadTokenDuration);
			CachedEngagerCount = FSquadCoordinator::Get().GetEngagerCount(Target, Now);
			if (UAIBlackboardComponent* BB = Blackboard.Get())
			{
				BB->SetFloat(FName("SquadSlot"), static_cast<float>(CachedSquadSlot));
				BB->SetFloat(FName("EngagerCount"), static_cast<float>(CachedEngagerCount));
			}
		}
	}
	else
	{
		CachedSquadSlot    = -1;
		CachedEngagerCount = 0;
	}
}

FVector AEnemyCharacter::ComputeSlotLocation(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return GetActorLocation();
	}
	const FVector TargetLoc    = Target->GetActorLocation();
	const float   AttackRange  = DefaultAttackRange;
	// 링 반경은 캡슐 반경 합 이상으로 — 슬롯에 서도 타깃/서로 몸통이 겹치지 않는다.
	const float   SelfRadius   = GetActorCapsuleRadius(const_cast<AEnemyCharacter*>(this));
	const float   TargetRadius = GetActorCapsuleRadius(Target);
	const float   MinRing      = SelfRadius + TargetRadius + 0.1f;
	const float   RingRadius   = (std::max)(MinRing, AttackRange * CombatRingRadiusScale);
	const int32   Count        = (std::max)(1, CachedEngagerCount);
	const int32   Slot        = (CachedSquadSlot >= 0) ? CachedSquadSlot : 0;
	const float   Angle       = (2.0f * 3.14159265f) * (static_cast<float>(Slot) / static_cast<float>(Count));
	const FVector Dir(cosf(Angle), sinf(Angle), 0.0f);
	FVector Loc = TargetLoc + Dir * RingRadius;
	Loc.Z = TargetLoc.Z;
	return Loc;
}

void AEnemyCharacter::ApplySeparationSteering()
{
	if (!bCanMove || IsDead() || HasCurrentAttack())
	{
		return;
	}
	if (UCharacterStateMachineComponent* SM = GetStateMachine())
	{
		if (SM->IsMovementLocked())
		{
			return;
		}
	}
	if (!AIBrainComponent.IsValid() || !AIBrainComponent->HasValidTarget())
	{
		return;
	}
	UCombatStateComponent* Combat = GetCombatStateComponent();
	if (!Combat)
	{
		return;
	}
	// 분리 반경은 몸통 크기 인식 — 캡슐이 큰 적은 더 멀리서부터 서로 밀어낸다(경량 RVO).
	const float   EffectiveRadius = (std::max)(SeparationRadius, GetActorCapsuleRadius(this) * 2.2f);
	const FVector Separation = FCombatTargetRegistry::Get().ComputeSeparation(Combat, GetActorLocation(), EffectiveRadius);
	if (Separation.IsNearlyZero())
	{
		return;
	}
	const float Crowd = (std::min)(1.0f, Separation.Length());
	AddMovementInput(Separation.Normalized(), SeparationStrength * Crowd);
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
		Brain->AcquireNearestHostileTarget(TargetSearchRange);
	}
	return Brain->HasValidTarget();
}

void AEnemyCharacter::Brain_FaceTarget()
{
	FaceTarget(LastTickDelta);
}

float AEnemyCharacter::Brain_GetDistance() const
{
	return AIBrainComponent.IsValid() ? AIBrainComponent->GetFlatDistanceToTarget() : 9999.0f;
}

float AEnemyCharacter::Brain_GetAttackRange() const
{
	return DefaultAttackRange;
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


const FEnemyAttackData* AEnemyCharacter::GetAttackDataByIndex(int32 Index) const
{
	if (!AttackComponent.IsValid())
	{
		return nullptr;
	}
	const TArray<FEnemyAttackData>& Attacks = AttackComponent->Attacks;
	if (Index < 0 || Index >= static_cast<int32>(Attacks.size()))
	{
		return nullptr;
	}
	return &Attacks[Index];
}

bool AEnemyCharacter::Brain_SetSelectedAttack(const FName& AttackName)
{
	SelectedAttackName = FName::None;
	if (!AttackName.IsValid() || !AttackComponent.IsValid())
	{
		return false;
	}
	const FEnemyAttackData Attack = AttackComponent->FindAttackByName(AttackName);
	if (!Attack.AttackName.IsValid() || !CanUseAttackDataNow(Attack))
	{
		return false;
	}
	SelectedAttackName = Attack.AttackName;
	return true;
}

bool AEnemyCharacter::CanUseAttackDataNow(const FEnemyAttackData& Attack) const
{
	if (!AttackComponent.IsValid() || !AIBrainComponent.IsValid() || !AIBrainComponent->HasValidTarget())
	{
		return false;
	}
	// 몽타주는 필수 아님 — 미제작 데이터에서도 timed 공격으로 실행 가능(StartAttackExecution 폴백).
	if (!Attack.AttackName.IsValid() || Attack.Weight <= 0.0f)
	{
		return false;
	}
	const int32 Phase = GetCurrentAIPhase();
	if (Phase < Attack.MinPhase || Phase > Attack.MaxPhase)
	{
		return false;
	}
	const float Distance = AIBrainComponent->GetFlatDistanceToTarget();
	const float AllowedMaxRange = Attack.bIsGapCloser ? Attack.MaxRange * GapCloserRangeScale : Attack.MaxRange;
	if (Distance < Attack.MinRange || Distance > AllowedMaxRange)
	{
		return false;
	}
	const float VerticalDelta = AIBrainComponent->GetVerticalDeltaToTarget();
	const float VerticalTolerance = Attack.MaxVerticalDelta > 0.0f ? Attack.MaxVerticalDelta : AttackVerticalTolerance;
	if (VerticalTolerance > 0.0f && VerticalDelta > VerticalTolerance)
	{
		return false;
	}
	if (Attack.bRequiresTargetInFront && fabsf(AIBrainComponent->GetAngleToTarget()) > Attack.MaxAbsAngle)
	{
		return false;
	}
	return AttackComponent->GetGlobalCooldownRemaining() <= 0.0f && !AttackComponent->IsAttackOnCooldown(Attack.AttackName);
}

bool AEnemyCharacter::Brain_PlaySelectedAttack()
{
	if (!SelectedAttackName.IsValid() || !AttackComponent.IsValid())
	{
		return false;
	}
	const FEnemyAttackData Attack = AttackComponent->FindAttackByName(SelectedAttackName);
	if (!Attack.AttackName.IsValid() || !CanUseAttackDataNow(Attack))
	{
		return false;
	}
	const float TokenDuration = GetAttackExecutionDuration(Attack) + AttackTokenSafetyBuffer;
	if (!AcquireAttackTokenForDuration(TokenDuration))
	{
		return false;
	}
	if (!StartAttackExecution(Attack))
	{
		Brain_ReleaseAttackToken();
		return false;
	}
	AttackComponent->CommitAttackData(Attack);
	if (UAIBlackboardComponent* BB = Blackboard.Get())
	{
		BB->PushRecentMove(Attack.AttackName);
	}
	return true;
}

bool AEnemyCharacter::Brain_PlayAttackByName(const FName& AttackName)
{
	return Brain_SetSelectedAttack(AttackName) && Brain_PlaySelectedAttack();
}

float AEnemyCharacter::Brain_GetAbsAngle() const
{
	return AIBrainComponent.IsValid() ? fabsf(AIBrainComponent->GetAngleToTarget()) : 180.0f;
}

float AEnemyCharacter::Brain_GetVerticalDelta() const
{
	return AIBrainComponent.IsValid() ? AIBrainComponent->GetVerticalDeltaToTarget() : 9999.0f;
}

float AEnemyCharacter::Brain_GetSelfHealthRatio() const
{
	return GetCurrentHealthRatio();
}

float AEnemyCharacter::Brain_GetTargetHealthRatio() const
{
	return Blackboard.IsValid() ? Blackboard->GetFloat(FName("TargetHealth")) : 1.0f;
}

float AEnemyCharacter::Brain_GetTargetPostureRatio() const
{
	return Blackboard.IsValid() ? Blackboard->GetFloat(FName("TargetPosture")) : 1.0f;
}

bool AEnemyCharacter::Brain_CanSeeTarget() const
{
	return Blackboard.IsValid() && Blackboard->GetBool(FName("CanSee"));
}

bool AEnemyCharacter::Brain_HasLineOfSight() const
{
	return Blackboard.IsValid() && Blackboard->GetBool(FName("HasLOS"));
}

bool AEnemyCharacter::Brain_IsInProximity() const
{
	return Blackboard.IsValid() && Blackboard->GetBool(FName("InProximity"));
}

int32 AEnemyCharacter::Brain_GetPhase() const
{
	return GetCurrentAIPhase();
}

float AEnemyCharacter::Brain_GetStateTime() const
{
	return GetStateMachine() ? GetStateMachine()->GetTimeInState() : 0.0f;
}

int32 AEnemyCharacter::Brain_GetLODLevel() const
{
	return CurrentLODLevel;
}

int32 AEnemyCharacter::Brain_GetAttackCount() const
{
	return AttackComponent.IsValid() ? static_cast<int32>(AttackComponent->Attacks.size()) : 0;
}

FName AEnemyCharacter::Brain_GetAttackName(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->AttackName : FName::None;
}

bool AEnemyCharacter::Brain_CanUseAttackIndex(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack && CanUseAttackDataNow(*Attack);
}

float AEnemyCharacter::Brain_GetAttackBaseWeight(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->Weight : 0.0f;
}

float AEnemyCharacter::Brain_GetAttackPriority(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->Priority : 0.0f;
}

float AEnemyCharacter::Brain_GetAttackMinRange(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->MinRange : 0.0f;
}

float AEnemyCharacter::Brain_GetAttackMaxRange(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->MaxRange : 0.0f;
}

float AEnemyCharacter::Brain_GetAttackMaxAbsAngle(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->MaxAbsAngle : 0.0f;
}

float AEnemyCharacter::Brain_GetAttackMaxVerticalDelta(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->MaxVerticalDelta : 0.0f;
}

float AEnemyCharacter::Brain_GetAttackRepeatWeightScale(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->RepeatWeightScale : 1.0f;
}

FName AEnemyCharacter::Brain_GetAttackTacticTag(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->TacticTag : FName::None;
}

bool AEnemyCharacter::Brain_IsAttackGapCloser(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack && Attack->bIsGapCloser;
}

int32 AEnemyCharacter::Brain_GetAttackPerilousType(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? static_cast<int32>(Attack->PerilousType) : 0;
}

bool AEnemyCharacter::Brain_AttackRequiresPreviousAttack(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack && Attack->bRequiresPreviousAttack;
}

FName AEnemyCharacter::Brain_GetAttackRequiredPreviousAttack(int32 Index) const
{
	const FEnemyAttackData* Attack = GetAttackDataByIndex(Index);
	return Attack ? Attack->RequiredPreviousAttack : FName::None;
}

int32 AEnemyCharacter::Brain_GetRecentAttackRepeat(const FName& AttackName) const
{
	return Blackboard.IsValid() ? Blackboard->GetRecentMoveRepeat(AttackName) : 0;
}

FName AEnemyCharacter::Brain_GetLastAttackName() const
{
	return AttackComponent.IsValid() ? AttackComponent->GetLastAttackName() : FName::None;
}

void AEnemyCharacter::Brain_BeginDecisionTrace()
{
	if (UAIDecisionTraceComponent* Trace = DecisionTrace.Get())
	{
		Trace->BeginDecision(GetStateMachine() ? GetStateMachine()->GetState() : FName::None);
	}
}

void AEnemyCharacter::Brain_AddDecisionCandidate(const FName& ActionName, float Score)
{
	if (UAIDecisionTraceComponent* Trace = DecisionTrace.Get())
	{
		Trace->AddCandidate(ActionName, Score);
	}
}

void AEnemyCharacter::Brain_CommitDecisionTrace(const FName& ChosenAction)
{
	if (UAIDecisionTraceComponent* Trace = DecisionTrace.Get())
	{
		Trace->CommitDecision(ChosenAction);
	}
}

void AEnemyCharacter::Brain_Chase()
{
	UEnemyAIBrainComponent* Brain = AIBrainComponent.Get();
	if (!Brain || !Brain->HasValidTarget())
	{
		return;
	}
	SetRuntimeState(FName("Chase"));
	const float Acceptance = (std::max)(0.3f, DefaultAttackRange * 0.4f);
	FVector GoalLocation = ComputeSlotLocation(Brain->GetTarget());

	// 시야가 끊긴 상태에서는 actor 현재 위치가 아니라 마지막으로 본 위치로 이동한다.
	// 이렇게 해야 벽 너머 actor pointer를 직접 추적하면서 비정상적인 공격/회전/슬롯팅을 하지 않는다.
	if (UAIBlackboardComponent* BB = Blackboard.Get())
	{
		const bool bCanSee = BB->GetBool(FName("CanSee"));
		const bool bInProximity = BB->GetBool(FName("InProximity"));
		const bool bLastSeenValid = BB->GetBool(FName("LastSeenValid"));
		if (!bCanSee && !bInProximity && bLastSeenValid)
		{
			GoalLocation = BB->GetLastSeenLocation();
		}
	}

	if (!RequestMoveToLocation(GoalLocation, Acceptance, true) && !Brain->IsMoveActive())
	{
		StopEnemyMovement();
		//UE_LOG("[EnemyAI] Brain_Chase move failed. Enemy=%s Reason=%s", GetName().c_str(), Brain->GetLastMoveFailureReason().c_str());
	}
}

void AEnemyCharacter::Brain_Strafe()
{
	// 기존 Chase path 입력과 상태머신 Strafe 입력이 합쳐지지 않도록 pathfollowing만 끊는다.
	StopPathFollowingOnly();
	SetRuntimeState(FName("Strafe"));
}

void AEnemyCharacter::Brain_Reposition()
{
	// 기존 Chase path 입력과 상태머신 Retreat 입력이 합쳐지지 않도록 pathfollowing만 끊는다.
	StopPathFollowingOnly();
	SetRuntimeState(FName("Reposition"));
}

void AEnemyCharacter::Brain_Idle()
{
	SetRuntimeState(FName("Idle"));
	FSquadCoordinator::Get().ReleaseToken(this);   // 타깃 없음 → 토큰 반환
	FSquadCoordinator::Get().ReleaseEngager(this); // 링 슬롯도 반환
	StopEnemyMovement();
}

bool AEnemyCharacter::Brain_IsTargetActing() const
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
	USkeletalMeshComponent* Mesh = Target->GetComponentByClass<USkeletalMeshComponent>();
	UAnimInstance* Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
	// 몽타주가 재생 중이면 상대가 능동 동작(주로 공격) 중 — 보스가 반응 가드 타이밍으로 사용.
	return Anim && Anim->IsMontagePlaying(nullptr);
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
	return Brain->GetFlatDistanceToTarget() <= DefaultAttackRange * 1.8f;
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
	StopEnemyMovement();
	if (UCombatStateComponent* Combat = GetCombatStateComponent())
	{
		Combat->OpenDeflectWindow(0.0f); // 0 → 컴포넌트 기본 윈도우 길이
	}
	SetRuntimeState(FName("Deflect"));
}

void AEnemyCharacter::Brain_OpenGuard()
{
	StopEnemyMovement();
	if (UCombatStateComponent* Combat = GetCombatStateComponent())
	{
		Combat->OpenGuardWindow(0.0f); // 0 → 컴포넌트 기본 가드 윈도우 길이(매 틱 호출 시 갱신)
	}
	SetRuntimeState(FName("Guard"));
	// 가드 비주얼 — 막기 자세 몽타주. 단, 이미 재생 중이면 재시작하지 않는다(매 틱 가드가
	// 선택돼도 블렌드인이 리셋되어 자세가 안 보이던 문제 수정).
	if (GuardMontage)
	{
		UAnimInstance* Anim = GetAnimInstance();
		if (Anim && !Anim->IsMontagePlaying(GuardMontage))
		{
			PlayCombatMontageRated(GuardMontage, 1.0f, 0.15f);
		}
	}
}

bool AEnemyCharacter::Brain_IsGuarding() const
{
	const UCombatStateComponent* Combat = GetCombatStateComponent();
	return Combat && Combat->IsGuarding();
}

void AEnemyCharacter::Brain_Dodge()
{
	StopEnemyMovement();
	SetRuntimeState(FName("Dodge"));
	// 타깃 반대로 빠지는 백스텝 — BeginDash 는 양수 스케일이면 타깃을 향하므로 음수를 줘서 멀어진다.
	if (UCharacterStateMachineComponent* SM = GetStateMachine())
	{
		const float DashSeconds = 0.35f;
		SM->BeginDash(DashSeconds, -DodgeDashScale);
	}
	if (DodgeMontage)
	{
		PlayCombatMontageRated(DodgeMontage, 1.2f, 0.10f);
	}
}

void AEnemyCharacter::Brain_LeapBack()
{
	StopEnemyMovement();
	SetRuntimeState(FName("Dodge"));
	// 강한 후방 대시(타깃 반대) + 점프(수직 팝) → 멀리 도약. BeginDash 는 양수면 타깃 방향이므로 음수.
	if (UCharacterStateMachineComponent* SM = GetStateMachine())
	{
		SM->BeginDash(0.45f, -LeapDashScale);
	}
	Jump();  // ACharacter::Jump — JumpZVelocity 만큼 수직 속도 부여 → 슬라이드가 아닌 도약
	if (LeapMontage)
	{
		PlayCombatMontageRated(LeapMontage, 1.0f, 0.08f);
	}
	else if (DodgeMontage)
	{
		PlayCombatMontageRated(DodgeMontage, 1.0f, 0.08f);
	}
}

void AEnemyCharacter::Brain_DriveLocomotion()
{
	UAnimInstance* Anim = GetAnimInstance();
	if (!Anim)
	{
		return;
	}
	float Speed = 0.0f;
	if (UCharacterMovementComponent* Move = GetComponentByClass<UCharacterMovementComponent>())
	{
		const FVector V = Move->GetVelocityValue();
		Speed = std::sqrt(V.X * V.X + V.Y * V.Y);
	}

	UAnimMontage* Desired = LocoIdleMontage;
	if (Speed >= LocoRunSpeedThreshold && LocoRunMontage)
	{
		Desired = LocoRunMontage;
	}
	else if (Speed >= LocoWalkSpeedThreshold && LocoWalkMontage)
	{
		Desired = LocoWalkMontage;
	}

	if (!Desired || Anim->IsMontagePlaying(Desired))
	{
		return; // 이미 맞는 로코모션 몽타주가 재생 중 — no-op
	}

	// 양보 규칙: 리액션 몽타주(피격/가드/회피)는 절대 덮어쓰지 않고 양보한다. 단, "내 공격
	// 몽타주의 꼬리(복귀 구간)"는 이동으로 전환할 때 걷기로 덮어써야 한다(예전엔 양보해서
	// 공격 포즈로 슬라이딩했다). 즉 비-로코·비-내공격 몽타주가 있을 때만 양보.
	const bool bLocoActive =
		(LocoIdleMontage && Anim->IsMontagePlaying(LocoIdleMontage)) ||
		(LocoWalkMontage && Anim->IsMontagePlaying(LocoWalkMontage)) ||
		(LocoRunMontage  && Anim->IsMontagePlaying(LocoRunMontage));
	const bool bAttackTail = CurrentAttackMontage && Anim->IsMontagePlaying(CurrentAttackMontage);
	if (Anim->IsMontagePlaying(nullptr) && !bLocoActive && !bAttackTail)
	{
		return; // 리액션(피격/가드/회피) 몽타주 진행 중 → 양보
	}

	Anim->PlayMontage(Desired, FName::None, 1.0f, 0.2f);
}

float AEnemyCharacter::Brain_GetClearanceInDirection(int32 Direction) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 9999.0f;
	}
	FVector Dir;
	switch (Direction)
	{
	case 1:  Dir = GetActorForward() * -1.0f; break; // 뒤
	case 2:  Dir = GetActorRight()   * -1.0f; break; // 좌
	case 3:  Dir = GetActorRight();          break;  // 우
	case 0:
	default: Dir = GetActorForward();        break;  // 앞
	}
	Dir.Z = 0.0f;
	if (Dir.IsNearlyZero())
	{
		return 9999.0f;
	}
	Dir = Dir.Normalized();

	const float MaxDist = 12.0f;
	FVector Start = GetActorLocation();
	Start.Z += 1.0f; // 몸통 높이 — 바닥을 때리지 않도록

	FHitResult Hit;
	if (World->PhysicsRaycast(Start, Dir, MaxDist, Hit, ECollisionChannel::WorldStatic, this))
	{
		return (std::max)(0.0f, Hit.Distance);
	}
	return MaxDist;
}

float AEnemyCharacter::Brain_GetBackClearance() const
{
	return Brain_GetClearanceInDirection(1);
}

bool AEnemyCharacter::Brain_IsCornered() const
{
	return Brain_GetBackClearance() <= CorneredClearanceThreshold;
}

// 은신 인지 노출 ──
int32 AEnemyCharacter::Brain_GetAwarenessState() const
{
	return Awareness.IsValid() ? Awareness->GetAwarenessStateInt() : 3; // 없으면 Alert 로 간주
}

float AEnemyCharacter::Brain_GetSuspicion() const
{
	return Awareness.IsValid() ? Awareness->GetSuspicion() : 1.0f;
}

bool AEnemyCharacter::Brain_IsAlerted() const
{
	// 게이팅이 꺼져 있으면 항상 전투 허용(기존 동작 보존). 켜져 있으면 Awareness 가 Alert 일 때만.
	if (!bUseAwarenessGating)
	{
		return true;
	}
	return Awareness.IsValid() ? Awareness->IsInCombat() : true;
}

bool AEnemyCharacter::Brain_IsUnaware() const
{
	if (!bUseAwarenessGating)
	{
		return false;
	}
	return Awareness.IsValid() ? Awareness->IsUnaware() : false;
}

void AEnemyCharacter::Brain_Investigate()
{
	if (!Awareness.IsValid())
	{
		return;
	}
	RequestMoveToLocation(Awareness->GetInvestigateLocation(), -1.0f, true);
}

// 공격 문법 분기 신호 ──
bool AEnemyCharacter::Brain_LastAttackWasDeflected() const
{
	return LastAttackResult == ECombatDamageResult::Deflected;
}

bool AEnemyCharacter::Brain_LastAttackConnected() const
{
	return bLastAttackConnected;
}

// 데스블로우 스톡 노출 ──
int32 AEnemyCharacter::Brain_GetStocks() const
{
	return Execution.IsValid() ? Execution->GetCurrentStocks() : 0;
}

bool AEnemyCharacter::Brain_IsDeathblowReady() const
{
	return Execution.IsValid() ? Execution->IsDeathblowReady() : false;
}

bool AEnemyCharacter::Brain_AcquireAttackToken()
{
	return AcquireAttackTokenForDuration(SquadTokenDuration);
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

int32 AEnemyCharacter::Brain_GetSquadRole() const
{
	// 링 슬롯(CachedSquadSlot)에서 파생. 0번=Closer(전열), 1번=Harasser(차열), 그 외/미교전=Reserve(후열).
	if (CachedSquadSlot < 0)  return 2; // 미교전 → Reserve
	if (CachedSquadSlot == 0) return 0; // Closer
	if (CachedSquadSlot == 1) return 1; // Harasser
	return 2;                            // Reserve
}

void AEnemyCharacter::UpdateAILOD()
{
	float Distance = 9999.0f;
	if (AIBrainComponent.IsValid() && AIBrainComponent->HasValidTarget())
	{
		Distance = AIBrainComponent->GetFlatDistanceToTarget();
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
