#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ProximaGameState.generated.h"

/** Replication-facing game state. Build/Live input mode remains local for now. */
UCLASS()
class PROXIMA_API AProximaGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AProximaGameState();
};
