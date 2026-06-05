#include "GameState.h"

FString AFinaleGameState::GetPhaseString()
{
	switch(Phase)
	{
	case EGamePhase::None:
	{
		return "None";
	}
	case EGamePhase::Playing:
	{
		return "Playing";
	}
	case EGamePhase::Paused:
	{
		return "Paused";
	}
	case EGamePhase::CutScene:
	{
		return "CutScene";
	}
	case EGamePhase::Dead:
	{
		return "Dead";
	}
	case EGamePhase::Defeated:
	{
		return "Defeated";
	}
	case EGamePhase::Victory:
	{
		return "Victory";
	}
	case EGamePhase::GameOver:
	{
		return "GameOver";
	}
	case EGamePhase::Leaderboard:
	{
		return "Leaderboard";
	}
	default:
	{
		return "Unregistered Phase";
	}
	}
}