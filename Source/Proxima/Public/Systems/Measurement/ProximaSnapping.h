#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProximaSnapping.generated.h"

/** Stateless grid snapping helpers. */
UCLASS()
class PROXIMA_API UProximaSnapping : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Proxima|Measurement")
    static float SnapToGrid(float ValueCm, float GridCm);
};
