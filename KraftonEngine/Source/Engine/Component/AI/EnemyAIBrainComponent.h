#pragma once

#include "Component/ActorComponent.h"
#include "Core/Delegate.h"
#include "Math/Vector.h"
#include "Navigation/NavTypes.h"
#include "Object/FName.h"
#include "Object/Ptr/WeakObjectPtr.h"

class AActor;

#include "Source/Engine/Component/AI/EnemyAIBrainComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FEnemyAIStateChangedSignature, class UEnemyAIBrainComponent*, FName, FName);
DECLARE_MULTICAST_DELEGATE_TwoParams(FEnemyAITargetChangedSignature, class UEnemyAIBrainComponent*, AActor*);

UCLASS()
class UEnemyAIBrainComponent : public UActorComponent
{
public:
	GENERATED_BODY()
	UEnemyAIBrainComponent() = default;
	~UEnemyAIBrainComponent() override = default;

	UFUNCTION(Callable, Category="EnemyAI|Target")
	void SetTarget(AActor* NewTarget);
	UFUNCTION(Callable, Category="EnemyAI|Target")
	void ClearTarget();
	UFUNCTION(Pure, Category="EnemyAI|Target")
	AActor* GetTarget() const { return TargetActor.Get(); }
	UFUNCTION(Pure, Category="EnemyAI|Target")
	bool HasValidTarget() const;

	UFUNCTION(Callable, Category="EnemyAI|Target")
	AActor* AcquireTargetByTag(const FName& Tag);
	UFUNCTION(Callable, Category="EnemyAI|Target")
	AActor* AcquireNearestHostileTarget(float SearchRange);
	UFUNCTION(Callable, Category="EnemyAI|Target")
	AActor* AcquireDefaultTarget();

	UFUNCTION(Pure, Category="EnemyAI|Target")
	float GetDistanceToTarget() const;
	UFUNCTION(Pure, Category="EnemyAI|Target")
	FVector GetDirectionToTarget() const;
	UFUNCTION(Pure, Category="EnemyAI|Target")
	FVector GetFlatDirectionToTarget() const;
	UFUNCTION(Pure, Category="EnemyAI|Target")
	float GetAngleToTarget() const;
	UFUNCTION(Pure, Category="EnemyAI|Target")
	bool IsTargetInRange(float Range) const;
	UFUNCTION(Pure, Category="EnemyAI|Target")
	bool IsTargetInFront(float MaxAbsAngleDegrees = 70.0f) const;
	UFUNCTION(Pure, Category="EnemyAI|Target")
	bool IsTargetBehind(float MinAbsAngleDegrees = 120.0f) const;

	UFUNCTION(Callable, Category="EnemyAI|State")
	void SetState(const FName& NewState);
	UFUNCTION(Pure, Category="EnemyAI|State")
	FName GetState() const { return CurrentState; }
	UFUNCTION(Pure, Category="EnemyAI|State")
	FName GetPreviousState() const { return PreviousState; }
	UFUNCTION(Pure, Category="EnemyAI|State")
	float GetStateTime() const { return StateTime; }

	UFUNCTION(Callable, Category="EnemyAI|Move")
	bool RequestMoveToTarget(float AcceptanceRadius = -1.0f, bool bUsePathfinding = true);
	UFUNCTION(Callable, Category="EnemyAI|Move")
	void StopMove();
	UFUNCTION(Pure, Category="EnemyAI|Move")
	bool IsMoveActive() const;
	UFUNCTION(Pure, Category="EnemyAI|Move")
	EPathFollowingStatus GetMoveStatus() const;
	UFUNCTION(Pure, Category="EnemyAI|Move")
	EPathFollowingRequestResult GetLastMoveRequestResult() const;
	UFUNCTION(Pure, Category="EnemyAI|Move")
	EPathFollowingResult GetLastMoveResult() const;
	UFUNCTION(Pure, Category="EnemyAI|Move")
	FString GetLastMoveFailureReason() const;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

	UPROPERTY(Edit, Save, Category="EnemyAI|Target", DisplayName="Default Target Tag")
	FName DefaultTargetTag = FName("Player");

	UPROPERTY(Edit, Save, Category="EnemyAI|Sense", DisplayName="Detection Range", Min=0.0f, Max=1000.0f, Speed=0.5f)
	float DetectionRange = 40.0f;

	UPROPERTY(Edit, Save, Category="EnemyAI|Sense", DisplayName="Lose Target Range", Min=0.0f, Max=1000.0f, Speed=0.5f)
	float LoseTargetRange = 55.0f;

	UPROPERTY(Edit, Save, Category="EnemyAI|Combat", DisplayName="Attack Range", Min=0.0f, Max=1000.0f, Speed=0.1f)
	float AttackRange = 3.0f;

	UPROPERTY(Edit, Save, Category="EnemyAI|State", DisplayName="Initial State")
	FName InitialState = FName("Idle");

	FEnemyAIStateChangedSignature OnStateChanged;
	FEnemyAITargetChangedSignature OnTargetChanged;

private:
	TWeakObjectPtr<AActor> TargetActor = nullptr;
	FName CurrentState = FName("Idle");
	FName PreviousState = FName::None;
	float StateTime = 0.0f;
	bool bHasInitializedState = false;
};
