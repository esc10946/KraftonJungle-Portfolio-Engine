#pragma once

#include "GameFramework/Pawn/BaseCombatCharacter.h"
#include "Component/Combat/CombatTypes.h"
#include "Object/Ptr/WeakObjectPtr.h"

class AAIController;
class UClass;
class UEnemyAIBrainComponent;
class UEnemyAttackComponent;
class ULuaScriptComponent;
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

protected:
	void Tick(float DeltaTime) override;
	void HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor) override;
	virtual int32 GetCurrentAIPhase() const { return 1; }
	virtual EEnemyAIBehaviorStyle GetResolvedBehaviorStyle() const { return BehaviorStyle; }
	void RebindEnemyComponents();
	void RunBuiltInDecisionLogic(float DeltaTime);
	void UpdateAttackExecution(float DeltaTime);
	bool StartAttackExecution(const FEnemyAttackData& Attack);
	float GetCurrentHealthRatio() const;
	bool IsTargetHostileDamageReceiver(AActor* Target) const;
	bool IsTargetInsideCurrentAttackFallback(AActor* Target) const;

	TWeakObjectPtr<UEnemyAIBrainComponent> AIBrainComponent = nullptr;
	TWeakObjectPtr<UEnemyAttackComponent> AttackComponent = nullptr;
	TWeakObjectPtr<ULuaScriptComponent> LuaScriptComponent = nullptr;

private:
	float ThinkTimer = 0.0f;
	float RepathTimer = 0.0f;
	bool bCurrentAttackActive = false;
	bool bFallbackDamageAttempted = false;
	float CurrentAttackElapsed = 0.0f;
	FEnemyAttackData CurrentAttack;
	TArray<TWeakObjectPtr<AActor>> CurrentAttackDamagedActors;
};
