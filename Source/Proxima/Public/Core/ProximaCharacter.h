#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProximaCharacter.generated.h"

UCLASS()
class PROXIMA_API AProximaCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AProximaCharacter();
    UPROPERTY(EditDefaultsOnly, Category = "Proxima")
    float WalkSpeed = 150.0f;
};
