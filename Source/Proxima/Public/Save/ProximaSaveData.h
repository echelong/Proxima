#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Building/ProximaPropertyData.h"
#include "Building/ProximaWallData.h"
#include "Systems/Measurement/ProximaMeasurementSubsystem.h"
#include "ProximaSaveData.generated.h"

UENUM(BlueprintType)
enum class EProximaSaveFormatVersion : uint8
{
    Invalid = 0 UMETA(Hidden),
    V1 = 1
};

USTRUCT(BlueprintType)
struct FProximaSaveHeader
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FName Format = FName(TEXT("ProximaSave"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    int32 Version = static_cast<int32>(EProximaSaveFormatVersion::V1);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FDateTime SaveTimeUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FString PropertyName;
};

/** Versioned save payload. Runtime Actors are never serialized into this object. */
UCLASS()
class PROXIMA_API UProximaSaveData : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaSaveHeader Header;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FProximaPropertyData> Properties;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FProximaBuildingData> Buildings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FProximaFloorData> Floors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FProximaWallData> Walls;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaMeasurementConfig MeasurementConfig;
};
