#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProximaExactLength.generated.h"

UCLASS()
class PROXIMA_API UProximaExactLength : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Proxima|Build")
    static FVector2D ResolveEndpointByLength(const FVector2D& StartCm, const FVector2D& DirectionNormalized, float RequestedLengthCm);
};