#pragma once

#include "GameFramework/Pawn/EnemyCharacter.h"
#include "Object/Ptr/WeakObjectPtr.h"

class UEncounterComponent;
class UPhaseComponent;
class FArchive;

#include "Source/Engine/GameFramework/Pawn/BossEnemyCharacter.generated.h"

UCLASS()
class ABossEnemyCharacter : public AEnemyCharacter
{
public:
	GENERATED_BODY()
	ABossEnemyCharacter() = default;
	~ABossEnemyCharacter() override = default;

	void InitDefaultComponents(const FString& SkeletalMeshFileName) override;
	void InitDefaultComponents(const FString& SkeletalMeshFileName, const FString& ScriptFile);
	void PostDuplicate() override;
	void OnPostLoad(FArchive& Ar) override;

	UFUNCTION(Pure, Category="Boss|Components")
	UPhaseComponent* GetPhaseComponent() const { return PhaseComponent; }
	UFUNCTION(Pure, Category="Boss|Components")
	UEncounterComponent* GetEncounterComponent() const { return EncounterComponent; }

	UFUNCTION(Callable, Category="Boss|Encounter")
	bool StartBossEncounter();
	UFUNCTION(Callable, Category="Boss|Encounter")
	bool CompleteBossEncounter();
	UFUNCTION(Pure, Category="Boss|Encounter")
	bool IsBossEncounterActive() const;

	UFUNCTION(Callable, Category="Boss|Phase")
	bool SetBossPhase(int32 NewPhase);
	UFUNCTION(Pure, Category="Boss|Phase")
	int32 GetBossPhase() const;
	UFUNCTION(Callable, Category="Boss|Phase")
	bool TryUpdatePhaseFromHealth();

protected:
	int32 GetCurrentAIPhase() const override { return GetBossPhase(); }
	EEnemyAIBehaviorStyle GetResolvedBehaviorStyle() const override;
	void HandleDamaged(UHealthComponent* Component, float Damage, float NewHealth, AActor* DamageCauser, AActor* InstigatorActor) override;
	void HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor) override;
	void HandlePhaseChanged(UPhaseComponent* Component, int32 OldPhase, int32 NewPhase);
	void HandleEncounterStarted(UEncounterComponent* Component);
	void HandleEncounterCompleted(UEncounterComponent* Component);
	void RebindBossComponents();

	TWeakObjectPtr<UPhaseComponent> PhaseComponent = nullptr;
	TWeakObjectPtr<UEncounterComponent> EncounterComponent = nullptr;
};
