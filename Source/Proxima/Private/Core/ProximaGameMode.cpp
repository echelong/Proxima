#include "Core/ProximaGameMode.h"
#include "Core/ProximaPlayerController.h"
#include "Core/ProximaCharacter.h"
#include "Core/ProximaGameState.h"

AProximaGameMode::AProximaGameMode()
{
    PlayerControllerClass = AProximaPlayerController::StaticClass();
    DefaultPawnClass = AProximaCharacter::StaticClass();
    GameStateClass = AProximaGameState::StaticClass();
}

void AProximaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
}
