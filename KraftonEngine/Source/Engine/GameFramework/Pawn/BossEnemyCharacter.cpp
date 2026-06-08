#include "GameFramework/Pawn/BossEnemyCharacter.h"

#include "Component/AI/EncounterComponent.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/PhaseComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/HealthComponent.h"
#include "Component/Particle/ParticleSystemComponent.h"

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
	return RequestBossPhase(NewPhase);
}

int32 ABossEnemyCharacter::GetBossPhase() const
{
	return PhaseComponent ? PhaseComponent->GetCurrentPhase() : 1;
}

void ABossEnemyCharacter::BeginExecutionPriorityWindow()
{
	if (!bExecutionPriorityWindowActive)
	{
		PendingExecutionPhase = 0;
	}
	bExecutionPriorityWindowActive = true;
}

void ABossEnemyCharacter::EndExecutionPriorityWindow()
{
	if (!bExecutionPriorityWindowActive)
	{
		return;
	}

	bExecutionPriorityWindowActive = false;
	ApplyDeferredBossPhaseChange();
}

bool ABossEnemyCharacter::RequestBossPhase(int32 NewPhase)
{
	if (!PhaseComponent || NewPhase <= PhaseComponent->GetCurrentPhase())
	{
		return false;
	}

	if (bExecutionPriorityWindowActive)
	{
		if (PendingExecutionPhase < NewPhase)
		{
			PendingExecutionPhase = NewPhase;
		}
		return true;
	}

	return PhaseComponent->SetPhase(NewPhase);
}

void ABossEnemyCharacter::ApplyDeferredBossPhaseChange()
{
	const int32 DeferredPhase = PendingExecutionPhase;
	PendingExecutionPhase = 0;

	if (!PhaseComponent || IsDead() || bExecutionPriorityWindowActive)
	{
		return;
	}

	if (DeferredPhase > PhaseComponent->GetCurrentPhase())
	{
		PhaseComponent->SetPhase(DeferredPhase);
	}
}

void ABossEnemyCharacter::HandleDamaged(UHealthComponent* Component, float Damage, float NewHealth, AActor* DamageCauser, AActor* InstigatorActor)
{
	Super::HandleDamaged(Component, Damage, NewHealth, DamageCauser, InstigatorActor);

	if (!Component || Component->IsDead())
	{
		return;
	}

	// Sekiro-style phase stock: health thresholds advance the boss grammar without
	// destroying/re-spawning the actor, so AI state, camera target, socket weapon, and
	// encounter ownership stay stable across phases.
	const float HealthRatio = Component->GetHealthRatio();
	int32 DesiredPhase = 1;
	if (HealthRatio <= 0.34f)
	{
		DesiredPhase = 3;
	}
	else if (HealthRatio <= 0.67f)
	{
		DesiredPhase = 2;
	}

	if (PhaseComponent && PhaseComponent->GetCurrentPhase() < DesiredPhase)
	{
		RequestBossPhase(DesiredPhase);
	}
}

void ABossEnemyCharacter::HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor)
{
	bExecutionPriorityWindowActive = false;
	PendingExecutionPhase = 0;
	Super::HandleDeath(Component, DamageCauser, InstigatorActor);
	CompleteBossEncounter();
}

void ABossEnemyCharacter::HandlePhaseChanged(UPhaseComponent* /*Component*/, int32 /*OldPhase*/, int32 NewPhase)
{
	SetAnimGraphInt(FName("Phase"), NewPhase);
	SetRuntimeState(FName("PhaseChanged"));

	// 페이즈 전환 연출: 자세변경 플러리시 몽타주 + 짧은 무적(연출 중 끊김 방지) + 파티클 버스트.
	if (PhaseChangeMontage)
	{
		PlayCombatMontage(PhaseChangeMontage);
	}
	if (UCombatStateComponent* Combat = GetCombatStateComponent())
	{
		Combat->OpenInvulnWindow(PhaseChangeInvulnSeconds);
	}
	// 연출 동안 두뇌를 busy 로 잠가 다음 행동(이동/공격/가드)이 전환 몽타주를 덮지 못하게 한다.
	LockBrainBusy(PhaseChangeInvulnSeconds);
	if (UParticleSystemComponent* PhaseVFX = GetComponentByClass<UParticleSystemComponent>())
	{
		PhaseVFX->Activate(true);
	}
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
