#pragma once

#include "CoreMinimal.h"
#include "ProximaSlabData.generated.h"

UENUM(BlueprintType)
enum class EProximaSlabKind : uint8 { Floor, FlatRoof };

/** A rectangular horizontal solid. Elevation is its top surface in centimetres. */
USTRUCT(BlueprintType)
struct FProximaSlabData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FGuid Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FVector2D MinCm = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FVector2D MaxCm = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float ElevationCm = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float ThicknessCm = 20.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    EProximaSlabKind Kind = EProximaSlabKind::Floor;

    bool IsValid() const;
};
