#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ProximaGameMode.generated.h"

UCLASS()
class PROXIMA_API AProximaGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AProximaGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
