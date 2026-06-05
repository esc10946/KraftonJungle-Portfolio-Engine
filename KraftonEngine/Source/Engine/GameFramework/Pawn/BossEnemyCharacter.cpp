#include "GameFramework/Pawn/BossEnemyCharacter.h"

#include "Component/AI/EncounterComponent.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/PhaseComponent.h"
#include "Component/Combat/HealthComponent.h"

void ABossEnemyCharacter::InitDefaultComponents(const FString& SkeletalMeshFileName)
{
	InitDefaultComponents(SkeletalMeshFileName, FString());
}

void ABossEnemyCharacter::InitDefaultComponents(const FString& SkeletalMeshFileName, const FString& ScriptFile)
{
	Super::InitDefaultComponents(SkeletalMeshFileName, ScriptFile);
	PhaseComponent = AddComponent<UPhaseComponent>();
	EncounterComponent = AddComponent<UEncounterComponent>();
	RebindBossComponents();
}

void ABossEnemyCharacter::PostDuplicate()
{
	Super::PostDuplicate();
	RebindBossComponents();
}

void ABossEnemyCharacter::OnPostLoad(FArchive& Ar)
{
	Super::OnPostLoad(Ar);
	RebindBossComponents();
}

void ABossEnemyCharacter::RebindBossComponents()
{
	PhaseComponent = GetComponentByClass<UPhaseComponent>();
	EncounterComponent = GetComponentByClass<UEncounterComponent>();
	if (PhaseComponent)
	{
		PhaseComponent->OnPhaseChanged.AddUObject(this, &ABossEnemyCharacter::HandlePhaseChanged);
	}
	if (EncounterComponent)
	{
		EncounterComponent->OnEncounterStarted.AddUObject(this, &ABossEnemyCharacter::HandleEncounterStarted);
		EncounterComponent->OnEncounterCompleted.AddUObject(this, &ABossEnemyCharacter::HandleEncounterCompleted);
	}
}

EEnemyAIBehaviorStyle ABossEnemyCharacter::GetResolvedBehaviorStyle() const
{
	return BehaviorStyle == EEnemyAIBehaviorStyle::Balanced ? EEnemyAIBehaviorStyle::Boss : BehaviorStyle;
}

bool ABossEnemyCharacter::StartBossEncounter()
{
	return EncounterComponent && EncounterComponent->StartEncounter();
}

bool ABossEnemyCharacter::CompleteBossEncounter()
{
	return EncounterComponent && EncounterComponent->CompleteEncounter();
}

bool ABossEnemyCharacter::IsBossEncounterActive() const
{
	return EncounterComponent && EncounterComponent->IsEncounterActive();
}

bool ABossEnemyCharacter::SetBossPhase(int32 NewPhase)
{
	return PhaseComponent && PhaseComponent->SetPhase(NewPhase);
}

int32 ABossEnemyCharacter::GetBossPhase() const
{
	return PhaseComponent ? PhaseComponent->GetCurrentPhase() : 1;
}

bool ABossEnemyCharacter::TryUpdatePhaseFromHealth()
{
	if (!PhaseComponent || !HealthComponent)
	{
		return false;
	}
	return PhaseComponent->TrySetPhaseByHealthRatio(HealthComponent->GetHealthRatio());
}

void ABossEnemyCharacter::HandleDamaged(UHealthComponent* Component, float Damage, float NewHealth, AActor* DamageCauser, AActor* InstigatorActor)
{
	Super::HandleDamaged(Component, Damage, NewHealth, DamageCauser, InstigatorActor);
	TryUpdatePhaseFromHealth();
}

void ABossEnemyCharacter::HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor)
{
	Super::HandleDeath(Component, DamageCauser, InstigatorActor);
	CompleteBossEncounter();
}

void ABossEnemyCharacter::HandlePhaseChanged(UPhaseComponent* /*Component*/, int32 /*OldPhase*/, int32 NewPhase)
{
	SetAnimGraphInt(FName("Phase"), NewPhase);
	if (AIBrainComponent)
	{
		AIBrainComponent->SetState(FName("PhaseChanged"));
	}
}

void ABossEnemyCharacter::HandleEncounterStarted(UEncounterComponent* /*Component*/)
{
	if (AIBrainComponent)
	{
		AIBrainComponent->SetState(FName("Intro"));
	}
}

void ABossEnemyCharacter::HandleEncounterCompleted(UEncounterComponent* /*Component*/)
{
	if (AIBrainComponent && !IsDead())
	{
		AIBrainComponent->SetState(FName("EncounterCompleted"));
	}
}
