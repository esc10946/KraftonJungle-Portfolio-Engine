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
enum class ECombatDamageResult : uint8
{
	None = 0,
	Damaged = 1,
	Killed = 2,
	Rejected = 3,
	// 방어자가 탄기(deflect) 윈도우 안에서 받아넘김 — 체력/체간 피해 무효, 공격자에게 체간 반사.
	// (직렬화 호환을 위해 끝에 추가)
	Deflected = 4,
};

inline const char* GCombatDamageResultNames[] = {
	"None",
	"Damaged",
	"Killed",
	"Rejected",
	"Deflected",
};
inline constexpr uint32 GCombatDamageResultCount = sizeof(GCombatDamageResultNames) / sizeof(GCombatDamageResultNames[0]);

// ── 세키로식 전투 분류 (Phase 2) ───────────────────────────────────────────────
// 위험공격(perilous) 종류. 방어자는 종류별로 다른 대응을 해야 한다(찌르기=간파/탄기,
// 하단=점프, 잡기=거리이탈).
UENUM()
enum class EPerilousType : uint8
{
	None = 0,
	Thrust = 1,   // 찌르기
	Sweep = 2,    // 하단베기
	Grab = 3,     // 잡기
};

inline const char* GPerilousTypeNames[] = {
	"None",
	"Thrust",
	"Sweep",
	"Grab",
};
inline constexpr uint32 GPerilousTypeCount = sizeof(GPerilousTypeNames) / sizeof(GPerilousTypeNames[0]);

// 탄기 등급 — 윈도우 안에서 얼마나 정확했는지.
UENUM()
enum class EDeflectGrade : uint8
{
	None = 0,
	Perfect = 1,
	Good = 2,
	Late = 3,
};

inline const char* GDeflectGradeNames[] = {
	"None",
	"Perfect",
	"Good",
	"Late",
};
inline constexpr uint32 GDeflectGradeCount = sizeof(GDeflectGradeNames) / sizeof(GDeflectGradeNames[0]);

// 공격 프레임 데이터 타임라인 단계.
UENUM()
enum class ECombatMovePhase : uint8
{
	Inactive = 0,
	Startup = 1,    // 선딜
	Active = 2,     // 활성(타격)
	Recovery = 3,   // 후딜(punish 대상)
};

inline const char* GCombatMovePhaseNames[] = {
	"Inactive",
	"Startup",
	"Active",
	"Recovery",
};
inline constexpr uint32 GCombatMovePhaseCount = sizeof(GCombatMovePhaseNames) / sizeof(GCombatMovePhaseNames[0]);

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

	// 수평 사거리와 별도로 허용할 최대 높이 차. 높낮이 맵에서 flat 거리만 보고
	// 벽 위/아래 타깃을 때리는 것을 막는다. 0 이하면 수직 제한을 적용하지 않는다.
	UPROPERTY(Edit, Save, Category="Attack|Range", DisplayName="Max Vertical Delta", Min=0.0f, Max=100000.0f, Speed=0.05f)
	float MaxVerticalDelta = 1.25f;

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


	// 전술 의미는 C++ enum이 아니라 Lua Blueprint/Data가 해석하는 태그다.
	UPROPERTY(Edit, Save, Category="Attack|Behavior", DisplayName="Tactic Tag")
	FName TacticTag = FName("Neutral");

	UPROPERTY(Edit, Save, Category="Attack|Behavior", DisplayName="Priority", Min=0.0f, Max=100.0f, Speed=0.1f)
	float Priority = 1.0f;

	UPROPERTY(Edit, Save, Category="Attack|Combo", DisplayName="Requires Previous Attack")
	bool bRequiresPreviousAttack = false;

	UPROPERTY(Edit, Save, Category="Attack|Combo", DisplayName="Required Previous Attack")
	FName RequiredPreviousAttack = FName::None;

	// ── 프레임 데이터 / 위험공격 분류 (Phase 2) ──
	// 60Hz 기준 프레임 수. 전투 시계가 이 타임라인을 진행시켜 위험표식·후딜 punish 창을 만든다.
	UPROPERTY(Edit, Save, Category="Attack|Frames", DisplayName="Startup Frames", Min=0.0f, Max=600.0f, Speed=1.0f)
	int32 StartupFrames = 12;

	UPROPERTY(Edit, Save, Category="Attack|Frames", DisplayName="Active Frames", Min=0.0f, Max=600.0f, Speed=1.0f)
	int32 ActiveFrames = 4;

	UPROPERTY(Edit, Save, Category="Attack|Frames", DisplayName="Recovery Frames", Min=0.0f, Max=600.0f, Speed=1.0f)
	int32 RecoveryFrames = 24;

	// 위험표식이 켜지는 선딜 내 프레임(텔레그래프 길이). PerilousType 이 None 이면 무시.
	UPROPERTY(Edit, Save, Category="Attack|Frames", DisplayName="Perilous Cue Frame", Min=0.0f, Max=600.0f, Speed=1.0f)
	int32 PerilousCueFrame = 6;

	UPROPERTY(Edit, Save, Category="Attack|Perilous", DisplayName="Perilous Type", Enum=EPerilousType)
	EPerilousType PerilousType = EPerilousType::None;

	UPROPERTY(Edit, Save, Category="Attack|Perilous", DisplayName="Can Be Deflected")
	bool bCanBeDeflected = true;
};

USTRUCT()
struct FEnemyPhaseData
{
	GENERATED_BODY()

	UPROPERTY(Edit, Save, Category="Phase", DisplayName="Phase")
	int32 Phase = 1;

	UPROPERTY(Edit, Save, Category="Phase", DisplayName="Phase Name")
	FName PhaseName = FName::None;

};
