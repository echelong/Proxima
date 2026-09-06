#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProximaSerializationUtility.generated.h"

UCLASS()
class PROXIMA_API UProximaSerializationUtility : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSaveFormatVersion = 1;

    UFUNCTION(BlueprintPure, Category = "Proxima|Serialization")
    static int32 GetSaveFormatVersion() { return CurrentSaveFormatVersion; }

    UFUNCTION(BlueprintPure, Category = "Proxima|Serialization")
    static bool ValidateSaveVersion(int32 Version);
};
