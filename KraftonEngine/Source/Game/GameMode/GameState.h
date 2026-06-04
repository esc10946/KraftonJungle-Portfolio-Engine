#pragma once
#include "Engine/GameFramework/GameMode/GameStateBase.h"

#include "Source/Game/GameMode/GameState.generated.h"

UENUM()
enum class EGamePhase : uint8 
{
	Title,
	Options,
	Playing,
	Paused,
	Dead,
	Victory,
	GameOver,
};

UCLASS()
class AFinaleGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	EGamePhase Phase = EGamePhase::Title;

};