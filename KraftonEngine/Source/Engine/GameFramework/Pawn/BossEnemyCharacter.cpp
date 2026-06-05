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

void ABossEnemyCharacter::HandleDamaged(UHealthComponent* Component, float Damage, float NewHealth, AActor* DamageCauser, AActor* InstigatorActor)
{
	Super::HandleDamaged(Component, Damage, NewHealth, DamageCauser, InstigatorActor);
}

void ABossEnemyCharacter::HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor)
{
	Super::HandleDeath(Component, DamageCauser, InstigatorActor);
	CompleteBossEncounter();
}

void ABossEnemyCharacter::HandlePhaseChanged(UPhaseComponent* /*Component*/, int32 /*OldPhase*/, int32 NewPhase)
{
	SetAnimGraphInt(FName("Phase"), NewPhase);
	SetRuntimeState(FName("PhaseChanged"));
}

void ABossEnemyCharacter::HandleEncounterStarted(UEncounterComponent* /*Component*/)
{
	SetRuntimeState(FName("Intro"));
}

void ABossEnemyCharacter::HandleEncounterCompleted(UEncounterComponent* /*Component*/)
{
	if (!IsDead())
	{
		SetRuntimeState(FName("EncounterCompleted"));
	}
}
