#pragma once

#include "GameFramework/Pawn/BaseCombatCharacter.h"
#include "Component/Combat/CombatTypes.h"
#include "AI/CombatClock.h"
#include "Object/Ptr/WeakObjectPtr.h"

class AAIController;
class UClass;
class UEnemyAIBrainComponent;
class UEnemyAttackComponent;
class ULuaScriptComponent;
class UAIBlackboardComponent;
class UAIPerceptionComponent;
class UUtilityReasonerComponent;
class UAIDecisionTraceComponent;
class UCombatMoveComponent;
class FArchive;

#include "Source/Engine/GameFramework/Pawn/EnemyCharacter.generated.h"

UCLASS()
class AEnemyCharacter : public ABaseCombatCharacter
{
public:
	GENERATED_BODY()
	AEnemyCharacter() = default;
	~AEnemyCharacter() override = default;

	void BeginPlay() override;
	void InitDefaultComponents(const FString& SkeletalMeshFileName) override;
	void InitDefaultComponents(const FString& SkeletalMeshFileName, const FString& ScriptFile);
	void PostDuplicate() override;
	void OnPostLoad(FArchive& Ar) override;

	UFUNCTION(Pure, Category="Enemy|Components")
	UEnemyAIBrainComponent* GetAIBrainComponent() const { return AIBrainComponent; }
	UFUNCTION(Pure, Category="Enemy|Components")
	UEnemyAttackComponent* GetAttackComponent() const { return AttackComponent; }
	UFUNCTION(Pure, Category="Enemy|Components")
	ULuaScriptComponent* GetLuaScriptComponent() const { return LuaScriptComponent; }

	// ── 세키로식 전투 AI 계층 (엔진 코어 컴포넌트) ──
	UFUNCTION(Pure, Category="Enemy|AICore")
	UAIBlackboardComponent* GetBlackboard() const { return Blackboard; }
	UFUNCTION(Pure, Category="Enemy|AICore")
	UAIPerceptionComponent* GetPerception() const { return Perception; }
	UFUNCTION(Pure, Category="Enemy|AICore")
	UUtilityReasonerComponent* GetReasoner() const { return Reasoner; }
	UFUNCTION(Pure, Category="Enemy|AICore")
	UAIDecisionTraceComponent* GetDecisionTrace() const { return DecisionTrace; }
	UFUNCTION(Pure, Category="Enemy|AICore")
	UCombatMoveComponent* GetCombatMove() const { return CombatMove; }
	UFUNCTION(Pure, Category="Enemy|AICore")
	int32 GetAIPhase() const { return GetCurrentAIPhase(); }

	UFUNCTION(Callable, Category="Enemy|AI")
	AAIController* SpawnDefaultAIController();
	UFUNCTION(Pure, Category="Enemy|AI")
	AAIController* GetEnemyAIController() const;

	UFUNCTION(Callable, Category="Enemy|Movement")
	void MoveToTarget(float Scale = 1.0f);
	UFUNCTION(Callable, Category="Enemy|Movement")
	void FaceTarget(float DeltaTime, float OverrideYawRate = -1.0f);
	UFUNCTION(Callable, Category="Enemy|Movement")
	void StopEnemyMovement();
	// 중립 거리 싸움용 동사 — Lua 두뇌가 호출한다. 타깃 기준 좌/우 게걸음, 타깃 반대로 후퇴.
	UFUNCTION(Callable, Category="Enemy|Movement")
	void StrafeAroundTarget(float Scale = 1.0f, bool bClockwise = true);
	UFUNCTION(Callable, Category="Enemy|Movement")
	void RetreatFromTarget(float Scale = 1.0f);
	UFUNCTION(Callable, Category="Enemy|Movement")
	bool RequestMoveToTarget(float AcceptanceRadius = -1.0f, bool bUsePathfinding = true);
	UFUNCTION(Callable, Category="Enemy|Movement")
	bool RequestMoveToActor(AActor* Target, float AcceptanceRadius = -1.0f, bool bUsePathfinding = true);
	UFUNCTION(Callable, Category="Enemy|Movement")
	bool RequestMoveToLocation(const FVector& Location, float AcceptanceRadius = -1.0f, bool bUsePathfinding = true);
	UFUNCTION(Pure, Category="Enemy|Movement")
	bool IsPathFollowing() const;

	UFUNCTION(Callable, Category="Enemy|Attack")
	bool SelectAndCommitAttack(int32 Phase, FEnemyAttackData& OutAttack);
	UFUNCTION(Callable, Category="Enemy|Attack")
	bool PlayAttackMontage(const FEnemyAttackData& Attack);
	UFUNCTION(Pure, Category="Enemy|Attack")
	bool HasCurrentAttack() const { return bCurrentAttackActive && CurrentAttack.AttackName.IsValid(); }
	UFUNCTION(Pure, Category="Enemy|Attack")
	bool HasCurrentAttackDamagedActor(AActor* Target) const;
	UFUNCTION(Callable, Category="Enemy|Attack")
	bool MarkCurrentAttackDamagedActor(AActor* Target);
	UFUNCTION(Callable, Category="Enemy|Attack")
	bool ApplyCurrentAttackDamageToActor(AActor* Target, const FVector& HitLocation);

	// ── Brain 동사 (Lua Blueprint 정책이 호출하는 파사드 — 결정은 Lua, 실행은 C++) ──
	// 모두 무인자/단순 반환이라 블루프린트 그래프에서 깔끔히 배선된다.
	UFUNCTION(Callable, Category="Enemy|Brain")
	void Brain_Sense();
	UFUNCTION(Callable, Category="Enemy|Brain")
	bool Brain_AcquireTarget();
	UFUNCTION(Callable, Category="Enemy|Brain")
	void Brain_FaceTarget();
	UFUNCTION(Pure, Category="Enemy|Brain")
	float Brain_GetDistance() const;
	UFUNCTION(Pure, Category="Enemy|Brain")
	float Brain_GetAttackRange() const;
	UFUNCTION(Callable, Category="Enemy|Brain")
	bool Brain_ConsumeCombatStep();
	UFUNCTION(Pure, Category="Enemy|Brain")
	bool Brain_IsBusy() const;
	UFUNCTION(Callable, Category="Enemy|Brain")
	bool Brain_SelectAttack();
	UFUNCTION(Callable, Category="Enemy|Brain")
	bool Brain_PlaySelectedAttack();
	UFUNCTION(Callable, Category="Enemy|Brain")
	void Brain_Chase();
	UFUNCTION(Callable, Category="Enemy|Brain")
	void Brain_Strafe();
	UFUNCTION(Callable, Category="Enemy|Brain")
	void Brain_Reposition();
	UFUNCTION(Callable, Category="Enemy|Brain")
	void Brain_Idle();

	// ── Phase 2 반응 동사 (체간/탄기/위험공격) ──
	// 타깃이 공격을 커밋했고 사정권이면 탄기 반응을 고려할 근거.
	UFUNCTION(Pure, Category="Enemy|Brain")
	bool Brain_TargetThreatening() const;
	// 타깃이 후딜(Recovery) 중인가 — punish 기회.
	UFUNCTION(Pure, Category="Enemy|Brain")
	bool Brain_TargetInRecovery() const;
	// 타깃의 활성 위험공격 종류(EPerilousType, 0=None).
	UFUNCTION(Pure, Category="Enemy|Brain")
	int32 Brain_GetTargetPerilous() const;
	// 탄기 윈도우를 연다(방어자 반응). 들어오는 피격이 윈도우 안이면 받아넘긴다.
	UFUNCTION(Callable, Category="Enemy|Brain")
	void Brain_OpenDeflect();

	// ── Phase 3 협동 (공격 토큰) ──
	// 공격 토큰을 시도/획득. 타깃에 이미 동시공격 상한만큼 나가 있으면 false → 지원/재배치.
	UFUNCTION(Callable, Category="Enemy|Brain")
	bool Brain_AcquireAttackToken();
	UFUNCTION(Callable, Category="Enemy|Brain")
	void Brain_ReleaseAttackToken();
	UFUNCTION(Pure, Category="Enemy|Brain")
	int32 Brain_GetSquadSlot() const;

	UPROPERTY(Edit, Save, Category="Enemy|Movement", DisplayName="Can Move")
	bool bCanMove = true;
	UPROPERTY(Edit, Save, Category="Enemy|Movement", DisplayName="Can Rotate")
	bool bCanRotate = true;
	UPROPERTY(Edit, Save, Category="Enemy|Attack", DisplayName="Can Attack")
	bool bCanAttack = true;
	UPROPERTY(Edit, Save, Category="Enemy|Movement", DisplayName="Turn Speed", Min=0.0f, Max=3600.0f, Speed=5.0f)
	float TurnSpeed = 540.0f;
	UPROPERTY(Edit, Save, Category="Enemy|AI", DisplayName="Auto Possess AI")
	bool bAutoPossessAI = true;
	UPROPERTY(Edit, Save, Category="Enemy|AI", DisplayName="AI Controller Class", Type=ClassRef, AllowedClass=AAIController)
	UClass* AIControllerClass = nullptr;

	UPROPERTY(Edit, Save, Category="Enemy|AI", DisplayName="Use Built-In Decision Logic")
	bool bUseBuiltInDecisionLogic = true;
	UPROPERTY(Edit, Save, Category="Enemy|AI", DisplayName="Behavior Style", Enum=EEnemyAIBehaviorStyle)
	EEnemyAIBehaviorStyle BehaviorStyle = EEnemyAIBehaviorStyle::Balanced;
	UPROPERTY(Edit, Save, Category="Enemy|AI", DisplayName="Think Interval", Min=0.0f, Max=2.0f, Speed=0.01f)
	float ThinkInterval = 0.12f;
	UPROPERTY(Edit, Save, Category="Enemy|AI", DisplayName="Path Repath Interval", Min=0.0f, Max=5.0f, Speed=0.05f)
	float RepathInterval = 0.35f;
	UPROPERTY(Edit, Save, Category="Enemy|AI", DisplayName="Defensive Retreat Distance", Min=0.0f, Max=1000.0f, Speed=0.1f)
	float DefensiveRetreatDistance = 1.4f;
	UPROPERTY(Edit, Save, Category="Enemy|AI", DisplayName="Defensive Retreat Input Scale", Min=0.0f, Max=1.0f, Speed=0.05f)
	float DefensiveRetreatInputScale = 0.55f;

	// ── Phase 3 협동 / Phase 4 LOD 튜닝 ──
	UPROPERTY(Edit, Save, Category="Enemy|Squad", DisplayName="Max Simultaneous Attackers", Min=0.0f, Max=16.0f, Speed=1.0f)
	int32 SquadMaxSimultaneousAttackers = 2;
	UPROPERTY(Edit, Save, Category="Enemy|Squad", DisplayName="Squad Token Duration", Min=0.0f, Max=10.0f, Speed=0.05f)
	float SquadTokenDuration = 1.0f;
	UPROPERTY(Edit, Save, Category="Enemy|LOD", DisplayName="LOD Near Distance", Min=0.0f, Max=1000.0f, Speed=0.5f)
	float LODNearDistance = 25.0f;
	UPROPERTY(Edit, Save, Category="Enemy|LOD", DisplayName="LOD Far Distance", Min=0.0f, Max=2000.0f, Speed=0.5f)
	float LODFarDistance = 80.0f;

protected:
	void Tick(float DeltaTime) override;
	void HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor) override;
	virtual int32 GetCurrentAIPhase() const { return 1; }
	virtual EEnemyAIBehaviorStyle GetResolvedBehaviorStyle() const { return BehaviorStyle; }
	void RebindEnemyComponents();
	void RunBuiltInDecisionLogic(float DeltaTime);
	void UpdateAttackExecution(float DeltaTime);
	// Phase 4: 타깃과의 거리로 LOD 를 정해 전투 시계 스텝 주기를 조절(원거리 적은 저빈도 think).
	void UpdateAILOD();
	bool StartAttackExecution(const FEnemyAttackData& Attack);
	float GetCurrentHealthRatio() const;
	bool IsTargetHostileDamageReceiver(AActor* Target) const;
	bool IsTargetInsideCurrentAttackFallback(AActor* Target) const;

	TWeakObjectPtr<UEnemyAIBrainComponent> AIBrainComponent = nullptr;
	TWeakObjectPtr<UEnemyAttackComponent> AttackComponent = nullptr;
	TWeakObjectPtr<ULuaScriptComponent> LuaScriptComponent = nullptr;
	TWeakObjectPtr<UAIBlackboardComponent> Blackboard = nullptr;
	TWeakObjectPtr<UAIPerceptionComponent> Perception = nullptr;
	TWeakObjectPtr<UUtilityReasonerComponent> Reasoner = nullptr;
	TWeakObjectPtr<UAIDecisionTraceComponent> DecisionTrace = nullptr;
	TWeakObjectPtr<UCombatMoveComponent> CombatMove = nullptr;

private:
	float ThinkTimer = 0.0f;
	float RepathTimer = 0.0f;
	bool bCurrentAttackActive = false;
	bool bFallbackDamageAttempted = false;
	float CurrentAttackElapsed = 0.0f;
	FEnemyAttackData CurrentAttack;
	TArray<TWeakObjectPtr<AActor>> CurrentAttackDamagedActors;

	// 전투 고정틱 시계 + Lua 두뇌 파사드 런타임 상태.
	FCombatClock CombatClock;
	float LastTickDelta = 0.0f;
	FName SelectedAttackName = FName::None;
	bool bStrafeClockwise = true;
	int32 CurrentLODLevel = 0; // 0=near(60Hz), 1=mid(30Hz), 2=far(10Hz)
};
