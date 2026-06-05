#pragma once
#include "Engine/GameFramework/GameMode/GameStateBase.h"

#include "Source/Game/GameMode/GameState.generated.h"

UENUM()
enum class EGamePhase : uint8 
{
	None,
	Playing,
	Paused,
	CutScene,
	Dead,
	Defeated,	// True death
	Victory,
	GameOver,
	Leaderboard,
};

UCLASS()
class AFinaleGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	EGamePhase Phase   = EGamePhase::None;
	uint16 ReviveCount = 0;
	uint16 Score	   = 0;

public:
	FString GetPhaseString();

};