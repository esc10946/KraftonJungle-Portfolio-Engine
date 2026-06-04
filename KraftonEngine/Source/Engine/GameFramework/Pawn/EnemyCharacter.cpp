#include "GameFramework/Pawn/EnemyCharacter.h"

#include "Animation/Montage/AnimMontage.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/EnemyAttackComponent.h"
#include "Component/Movement/CharacterMovementComponent.h"
#include "Component/Script/LuaScriptComponent.h"
#include "Math/MathUtils.h"
#include "Math/Rotator.h"

#include <cmath>

namespace
{
	float NormalizeDeltaYaw(float Delta)
	{
		while (Delta > 180.0f) Delta -= 360.0f;
		while (Delta < -180.0f) Delta += 360.0f;
		return Delta;
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
}

void AEnemyCharacter::PostDuplicate()
{
	Super::PostDuplicate();
	AIBrainComponent = GetComponentByClass<UEnemyAIBrainComponent>();
	AttackComponent = GetComponentByClass<UEnemyAttackComponent>();
	LuaScriptComponent = GetComponentByClass<ULuaScriptComponent>();
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
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->ClearInputVector();
		Movement->StopMovementImmediately();
	}
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
	OutAttack = AttackComponent->SelectAttack(Phase, Distance, AbsAngle);
	if (!OutAttack.AttackName.IsValid())
	{
		return false;
	}
	return AttackComponent->CommitAttackData(OutAttack);
}

bool AEnemyCharacter::PlayAttackMontage(const FEnemyAttackData& Attack)
{
	return PlayCombatMontage(Attack.Montage);
}

void AEnemyCharacter::HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor)
{
	Super::HandleDeath(Component, DamageCauser, InstigatorActor);
	bCanMove = false;
	bCanRotate = false;
	bCanAttack = false;
	StopEnemyMovement();
	if (AIBrainComponent)
	{
		AIBrainComponent->SetState(FName("Dead"));
	}
}
