#pragma once

#include "Core/Types/CoreTypes.h"
#include "Math/Vector.h"
#include "Object/FName.h"

class AActor;
class UAnimMontage;

#include "Source/Engine/Component/Combat/CombatTypes.generated.h"

UENUM()
enum class ECombatTeam : uint8
{
	Neutral = 0,
	Player = 1,
	Enemy = 2,
	World = 3,
};

inline const char* GCombatTeamNames[] = {
	"Neutral",
	"Player",
	"Enemy",
	"World",
};
inline constexpr uint32 GCombatTeamCount = sizeof(GCombatTeamNames) / sizeof(GCombatTeamNames[0]);


UENUM()
enum class EEnemyAIBehaviorStyle : uint8
{
	Passive = 0,
	Balanced = 1,
	Aggressive = 2,
	Defensive = 3,
	Boss = 4,
};

inline const char* GEnemyAIBehaviorStyleNames[] = {
	"Passive",
	"Balanced",
	"Aggressive",
	"Defensive",
	"Boss",
};
inline constexpr uint32 GEnemyAIBehaviorStyleCount = sizeof(GEnemyAIBehaviorStyleNames) / sizeof(GEnemyAIBehaviorStyleNames[0]);

UENUM()
enum class EEnemyAttackTactic : uint8
{
	Neutral = 0,
	Opener = 1,
	Pressure = 2,
	Combo = 3,
	GapCloser = 4,
	Punish = 5,
	Retreat = 6,
	PhaseChange = 7,
};

inline const char* GEnemyAttackTacticNames[] = {
	"Neutral",
	"Opener",
	"Pressure",
	"Combo",
	"GapCloser",
	"Punish",
	"Retreat",
	"PhaseChange",
};
inline constexpr uint32 GEnemyAttackTacticCount = sizeof(GEnemyAttackTacticNames) / sizeof(GEnemyAttackTacticNames[0]);

UENUM()
enum class ECombatDamageResult : uint8
{
	None = 0,
	Damaged = 1,
	Killed = 2,
	Rejected = 3,
};

inline const char* GCombatDamageResultNames[] = {
	"None",
	"Damaged",
	"Killed",
	"Rejected",
};
inline constexpr uint32 GCombatDamageResultCount = sizeof(GCombatDamageResultNames) / sizeof(GCombatDamageResultNames[0]);

USTRUCT()
struct FCombatDamageSpec
{
	GENERATED_BODY()

	UPROPERTY(Edit, Save, Category="Damage", DisplayName="Damage", Min=0.0f, Max=100000.0f, Speed=0.5f)
	float Damage = 10.0f;

	UPROPERTY(Edit, Save, Category="Damage", DisplayName="Poise Damage", Min=0.0f, Max=100000.0f, Speed=0.5f)
	float PoiseDamage = 10.0f;

	UPROPERTY(Transient, Category="Damage")
	AActor* DamageCauser = nullptr;

	UPROPERTY(Transient, Category="Damage")
	AActor* InstigatorActor = nullptr;

	UPROPERTY(Transient, Category="Damage")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(Transient, Category="Damage")
	FVector HitDirection = FVector::ZeroVector;
};

USTRUCT()
struct FCombatDamageReport
{
	GENERATED_BODY()

	UPROPERTY(Transient, Category="Damage")
	ECombatDamageResult Result = ECombatDamageResult::None;

	UPROPERTY(Transient, Category="Damage")
	float RequestedDamage = 0.0f;

	UPROPERTY(Transient, Category="Damage")
	float AppliedDamage = 0.0f;

	UPROPERTY(Transient, Category="Damage")
	float PreviousHealth = 0.0f;

	UPROPERTY(Transient, Category="Damage")
	float NewHealth = 0.0f;

	UPROPERTY(Transient, Category="Damage")
	bool bKilled = false;
};

USTRUCT()
struct FEnemyAttackData
{
	GENERATED_BODY()

	UPROPERTY(Edit, Save, Category="Attack", DisplayName="Attack Name")
	FName AttackName = FName::None;

	UPROPERTY(Edit, Save, Category="Attack", DisplayName="Montage", Type=ObjectRef, AllowedClass=UAnimMontage)
	UAnimMontage* Montage = nullptr;

	UPROPERTY(Edit, Save, Category="Attack|Phase", DisplayName="Min Phase", Min=0.0f, Max=99.0f, Speed=1.0f)
	int32 MinPhase = 1;

	UPROPERTY(Edit, Save, Category="Attack|Phase", DisplayName="Max Phase", Min=0.0f, Max=99.0f, Speed=1.0f)
	int32 MaxPhase = 1;

	UPROPERTY(Edit, Save, Category="Attack|Range", DisplayName="Min Range", Min=0.0f, Max=100000.0f, Speed=1.0f)
	float MinRange = 0.0f;

	UPROPERTY(Edit, Save, Category="Attack|Range", DisplayName="Max Range", Min=0.0f, Max=100000.0f, Speed=1.0f)
	float MaxRange = 300.0f;

	UPROPERTY(Edit, Save, Category="Attack|Angle", DisplayName="Max Abs Angle", Min=0.0f, Max=180.0f, Speed=1.0f)
	float MaxAbsAngle = 70.0f;

	UPROPERTY(Edit, Save, Category="Attack|Timing", DisplayName="Cooldown", Min=0.0f, Max=60.0f, Speed=0.05f)
	float Cooldown = 2.0f;

	UPROPERTY(Edit, Save, Category="Attack|Timing", DisplayName="Recovery Time", Min=0.0f, Max=60.0f, Speed=0.05f)
	float RecoveryTime = 0.8f;

	UPROPERTY(Edit, Save, Category="Attack|Weight", DisplayName="Weight", Min=0.0f, Max=1000.0f, Speed=0.1f)
	float Weight = 1.0f;

	UPROPERTY(Edit, Save, Category="Attack|Weight", DisplayName="Repeat Weight Scale", Min=0.0f, Max=1.0f, Speed=0.05f)
	float RepeatWeightScale = 0.25f;

	UPROPERTY(Edit, Save, Category="Attack|Damage", DisplayName="Damage", Min=0.0f, Max=100000.0f, Speed=0.5f)
	float Damage = 10.0f;

	UPROPERTY(Edit, Save, Category="Attack|Damage", DisplayName="Poise Damage", Min=0.0f, Max=100000.0f, Speed=0.5f)
	float PoiseDamage = 10.0f;

	UPROPERTY(Edit, Save, Category="Attack|Behavior", DisplayName="Requires Target In Front")
	bool bRequiresTargetInFront = true;

	UPROPERTY(Edit, Save, Category="Attack|Behavior", DisplayName="Gap Closer")
	bool bIsGapCloser = false;


	UPROPERTY(Edit, Save, Category="Attack|Behavior", DisplayName="Tactic", Enum=EEnemyAttackTactic)
	EEnemyAttackTactic Tactic = EEnemyAttackTactic::Neutral;

	UPROPERTY(Edit, Save, Category="Attack|Behavior", DisplayName="Priority", Min=0.0f, Max=100.0f, Speed=0.1f)
	float Priority = 1.0f;

	UPROPERTY(Edit, Save, Category="Attack|Combo", DisplayName="Requires Previous Attack")
	bool bRequiresPreviousAttack = false;

	UPROPERTY(Edit, Save, Category="Attack|Combo", DisplayName="Required Previous Attack")
	FName RequiredPreviousAttack = FName::None;

	UPROPERTY(Edit, Save, Category="Attack|FallbackHit", DisplayName="Use Fallback Damage")
	bool bUseFallbackDamage = true;

	UPROPERTY(Edit, Save, Category="Attack|FallbackHit", DisplayName="Fallback Damage Delay", Min=0.0f, Max=10.0f, Speed=0.01f)
	float FallbackDamageDelay = 0.35f;

	UPROPERTY(Edit, Save, Category="Attack|FallbackHit", DisplayName="Fallback Hit Radius", Min=0.0f, Max=100000.0f, Speed=0.5f)
	float FallbackHitRadius = 1.5f;
};

USTRUCT()
struct FEnemyPhaseData
{
	GENERATED_BODY()

	UPROPERTY(Edit, Save, Category="Phase", DisplayName="Phase")
	int32 Phase = 1;

	UPROPERTY(Edit, Save, Category="Phase", DisplayName="Phase Name")
	FName PhaseName = FName::None;

	UPROPERTY(Edit, Save, Category="Phase", DisplayName="Auto Enter At Or Below Health Ratio")
	bool bAutoEnterAtOrBelowHealthRatio = false;

	UPROPERTY(Edit, Save, Category="Phase", DisplayName="Health Ratio Threshold", Min=0.0f, Max=1.0f, Speed=0.01f)
	float HealthRatioThreshold = 1.0f;
};
