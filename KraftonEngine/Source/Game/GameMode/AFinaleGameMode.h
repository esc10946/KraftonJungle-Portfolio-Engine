#pragma once
#include "Engine/GameFramework/GameMode/GameModeBase.h"

UCLASS()
class AFinaleGameMode : public AGameModeBase
{
public:
	AFinaleGameMode();
	void StartMatch() override;
	void OnPossessedPawnEnteredTrigger(ATriggerVolumeBase* Trigger, APawn* Pawn) override;
	void OnPossessedPawnExitedTrigger(ATriggerVolumeBase* Trigger, APawn* Pawn)  override;

	// Change the scene to the target when called
	// TODO: Change the uint8 to enum when it rolls in
	void TransitScene(uint8 TargetScene);
	void OnPlayerDeath();
	void OnBossSlain(FName BossId);
	void OnVictory();

private:


};