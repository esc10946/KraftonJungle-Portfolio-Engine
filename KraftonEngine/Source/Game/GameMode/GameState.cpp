#include "GameState.h"

FString AFinaleGameState::GetPhaseString()
{
	switch(Phase)
	{
	case EGamePhase::None:
	{
		return "None";
		break;
	}
	case EGamePhase::Playing:
	{
		return "Playing";
		break;
	}
	case EGamePhase::Paused:
	{
		return "Paused";
		break;
	}
	case EGamePhase::CutScene:
	{
		return "CutScene";
		break;
	}
	case EGamePhase::Dead:
	{
		return "Dead";
		break;
	}
	case EGamePhase::Victory:
	{
		return "Victory";
		break;
	}
	case EGamePhase::GameOver:
	{
		return "GameOver";
		break;
	}
	case EGamePhase::Leaderboard:
	{
		return "Leaderboard";
		break;
	}
	default:
	{
		return "Unregistered Phase";
	}
	}
}