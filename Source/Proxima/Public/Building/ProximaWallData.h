#pragma once

#include "CoreMinimal.h"
#include "Building/ProximaIdentifiers.h"
#include "ProximaWallData.generated.h"

/** A point on the property's horizontal XY plane, expressed in centimeters. */
USTRUCT(BlueprintType)
struct FProximaWallPoint
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float XCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float YCm = 0.0f;

    FVector2D ToVector2D() const { return FVector2D(XCm, YCm); }

    bool operator==(const FProximaWallPoint& Other) const
    {
        return XCm == Other.XCm && YCm == Other.YCm;
    }
};

UENUM(BlueprintType)
enum class EProximaOpeningType : uint8
{
    Door,
    Window,
    Archway,
    Custom
};

/** A rectangular opening cut into a wall. */
USTRUCT(BlueprintType)
struct FProximaOpeningData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaOpeningID OpeningId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    EProximaOpeningType Type = EProximaOpeningType::Door;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float OffsetFromStartCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float WidthCm = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float BottomHeightCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float TopHeightCm = 210.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaCatalogID CatalogEntry;
};

/** Persistent wall model. Runtime wall actors are reconstructed from this data. */
USTRUCT(BlueprintType)
struct FProximaWallData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaWallID WallId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaBuildingID BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaFloorID FloorId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaWallPoint StartPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaWallPoint EndPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float HeightCm = 270.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float ThicknessCm = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaCatalogID SideAMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaCatalogID SideBMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FProximaOpeningData> Openings;

    // Explicit topology reference. Room detection must validate/rebuild these links as geometry changes.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FProximaWallID> ConnectedWalls;

    /** Checks finite dimensions and every opening, including overlap and duplicate IDs. */
    bool IsValid() const;
    float GetLengthCm() const;
    bool IsDegenerate(float ToleranceCm = 0.01f) const;
    FVector GetStartWorld(const FVector& PropertyOriginCm, float FloorElevationCm = 0.0f) const;
    FVector GetEndWorld(const FVector& PropertyOriginCm, float FloorElevationCm = 0.0f) const;
    FVector GetMidWorld(const FVector& PropertyOriginCm, float FloorElevationCm = 0.0f) const;
    FRotator GetRotation() const;
};
