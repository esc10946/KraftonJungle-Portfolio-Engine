#include "AFinaleGameMode.h"
#include "GameState.h"
#include "Engine/Core/Logging/Log.h"
#include "Engine/Object/Reflection/UClass.h"
#include "Engine/GameFramework/World.h"
#include "Engine/Runtime/Engine.h"

void AFinaleGameMode::SetGamePhase(EGamePhase InPhase)
{
	if (AFinaleGameState* State = Cast<AFinaleGameState>(GetGameState()))
	{
		UE_LOG("[AFinaleGameMode] Game Phase set to: %s", GetGamePhaseString().c_str());
		State->Phase = InPhase;
	}
}

bool AFinaleGameMode::CheckGamePhase(EGamePhase InPhase)
{
	if (AFinaleGameState* State = Cast<AFinaleGameState>(GetGameState()))
	{
		return State->Phase == InPhase;
	}

	return false;
}

FString AFinaleGameMode::GetGamePhaseString()
{
	if (AFinaleGameState* State = Cast<AFinaleGameState>(GetGameState()))
	{
		return State->GetPhaseString();
	}

	return "Unknown";
}

AFinaleGameMode::AFinaleGameMode()
{
	GameStateClass = AFinaleGameState::StaticClass();
}

void AFinaleGameMode::StartMatch()
{
	AGameModeBase::StartMatch();
	UE_LOG("[AFinaleGameMode] Match Started");
	SetGamePhase(EGamePhase::Playing);
}

void AFinaleGameMode::TogglePause()
{
	if (CheckGamePhase(EGamePhase::Playing))     OnGamePaused();
	else if (CheckGamePhase(EGamePhase::Paused)) OnGameResumed();
}

void AFinaleGameMode::OnGamePaused()
{
	if (!CheckGamePhase(EGamePhase::Playing)) return;

	SetGamePhase(EGamePhase::Paused);

	if (UWorld* World = GetWorld())
	{
		World->SetPaused(true);
	}

	// TODO: Show pause menu. Show mouse cursor
}

// Called by player controller or an optional UI button
void AFinaleGameMode::OnGameResumed()
{
	if (!CheckGamePhase(EGamePhase::Paused)) return;

	if (UWorld* World = GetWorld())
	{
		World->SetPaused(false);
	}

	SetGamePhase(EGamePhase::Playing);

	// TODO: Hide pause menu
}

void AFinaleGameMode::OnEnteringCutscene()
{
	// Block player input
	SetGamePhase(EGamePhase::CutScene);
}

void AFinaleGameMode::OnGameQuit()
{
	// Unfreeze before leaving — the title scene spawns a fresh World, but
	// don't hand off a paused one.
	if (UWorld* World = GetWorld())
	{
		World->SetPaused(false);
	}

	if (GEngine)
	{
		GEngine->RequestTransitionToScene("Game/GameTitle");
	}
}

void AFinaleGameMode::OnLeaderBoardView()
{

}

void AFinaleGameMode::OnPlayerDeath()
{
	if (!CheckGamePhase(EGamePhase::Playing)) return;
	SetGamePhase(EGamePhase::Dead);
}

void AFinaleGameMode::OnPlayerRevive()
{
	if (!CheckGamePhase(EGamePhase::Dead)) return;
	SetGamePhase(EGamePhase::Playing);
	if (AFinaleGameState* State = Cast<AFinaleGameState>(GetGameState()))
	{
		State->ReviveCount++;
	}

}

void AFinaleGameMode::OnPlayerDefeated()
{
	if (!CheckGamePhase(EGamePhase::Dead)) return;
	SetGamePhase(EGamePhase::Defeated);
}

void AFinaleGameMode::OnBossSlain(FName BossId)
{
	if (!CheckGamePhase(EGamePhase::Playing)) return;
}

void AFinaleGameMode::OnVictory()
{
	if (!CheckGamePhase(EGamePhase::Playing)) return;
	SetGamePhase(EGamePhase::Victory);
}

void AFinaleGameMode::OnPossessedPawnEnteredTrigger(ATriggerVolumeBase* Trigger, APawn* Pawn)
{
	// Does nothing for now
}

void AFinaleGameMode::OnPossessedPawnExitedTrigger(ATriggerVolumeBase* Trigger, APawn* Pawn)
{
	// Does nothing for now
}
