#pragma once

#include "CoreMinimal.h"
#include "Building/ProximaIdentifiers.h"
#include "ProximaStairData.generated.h"

/**
 * Axis-aligned rectangular floor opening.
 *
 * This is derived from a stair rather than independently persisted.
 */
USTRUCT(BlueprintType)
struct FProximaFloorOpeningRect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D MinCm =
        FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D MaxCm =
        FVector2D::ZeroVector;

    bool IsValid() const
    {
        return
            FMath::IsFinite(MinCm.X) &&
            FMath::IsFinite(MinCm.Y) &&
            FMath::IsFinite(MaxCm.X) &&
            FMath::IsFinite(MaxCm.Y) &&
            MaxCm.X - MinCm.X >
                0.1f &&
            MaxCm.Y - MinCm.Y >
                0.1f;
    }
};

/**
 * One straight stair flight connecting two adjacent storeys.
 *
 * StartCm is the lower entrance centre.
 * EndCm is the upper entrance centre.
 */
USTRUCT(BlueprintType)
struct FProximaStairData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FGuid StairId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaFloorID LowerFloorId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaFloorID UpperFloorId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FVector2D StartCm =
        FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FVector2D EndCm =
        FVector2D(
            420.0f,
            0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float WidthCm =
        100.0f;

    bool IsValid() const;

    float GetRunCm() const
    {
        return
            FVector2D::Distance(
                StartCm,
                EndCm);
    }

    FProximaFloorOpeningRect
    GetOpeningRect() const;
};
