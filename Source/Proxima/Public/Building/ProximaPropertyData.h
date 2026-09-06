#pragma once

#include "CoreMinimal.h"
#include "Building/ProximaIdentifiers.h"
#include "ProximaPropertyData.generated.h"

/** Persistent data for one buildable lot/property. All dimensions are centimeters. */
USTRUCT(BlueprintType)
struct FProximaPropertyData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaPropertyID PropertyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FString PropertyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FVector OriginCm = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FVector2D LotSizeCm = FVector2D(3000.0, 3000.0);
};

/** Persistent metadata for a building that belongs to a property. */
USTRUCT(BlueprintType)
struct FProximaBuildingData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaPropertyID PropertyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaBuildingID BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FString BuildingName;
};

/** Persistent storey/floor metadata. */
USTRUCT(BlueprintType)
struct FProximaFloorData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaFloorID FloorId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaBuildingID BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    int32 LevelIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float BaseElevationCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float StoreyHeightCm = 270.0f;
};
