#include "AFinaleGameMode.h"
#include "GameState.h"
#include "Engine/Core/Logging/Log.h"
#include "Engine/Object/Reflection/UClass.h"

AFinaleGameMode::AFinaleGameMode()
{
	GameStateClass = AFinaleGameState::StaticClass();
}

void AFinaleGameMode::StartMatch()
{
	UE_LOG("[AFinalGameMode] Match Started");
	AGameModeBase::StartMatch();
}

void AFinaleGameMode::OnPlayerDeath()
{
	UE_LOG("[AFinalGameMode] OnPlayerDeath called");
}

void AFinaleGameMode::OnBossSlain(FName BossId)
{
	UE_LOG("[AFinalGameMode] OnBossSlain called");
}

void AFinaleGameMode::OnVictory()
{
	UE_LOG("[AFinalGameMode] OnVictory called");
}

void AFinaleGameMode::OnPossessedPawnEnteredTrigger(ATriggerVolumeBase* Trigger, APawn* Pawn)
{
	// Does nothing for now
}

void AFinaleGameMode::OnPossessedPawnExitedTrigger(ATriggerVolumeBase* Trigger, APawn* Pawn)
{
	// Does nothing for now
}