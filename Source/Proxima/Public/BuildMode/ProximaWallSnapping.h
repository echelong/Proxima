#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProximaWallSnapping.generated.h"

UCLASS()
class PROXIMA_API UProximaWallSnapping : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Proxima|Build")
    static FVector2D SnapPointToGrid(const FVector2D& PointCm, float GridCm);

    UFUNCTION(BlueprintPure, Category = "Proxima|Build")
    static FVector2D SnapToNearestEndpoint(
        const FVector2D& CandidateCm,
        const TArray<FVector2D>& ExistingEndpointsCm,
        float ToleranceCm);

    static bool FindNearestEndpoint(
        const FVector2D& CandidateCm,
        const TArray<FVector2D>& ExistingEndpointsCm,
        float ToleranceCm,
        FVector2D& OutEndpointCm);
};
