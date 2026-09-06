#pragma once

#include "CoreMinimal.h"
#include "ProximaIdentifiers.generated.h"

/** Stable runtime/save identifier. GUIDs avoid collisions after loading old saves. */
USTRUCT(BlueprintType)
struct FProximaID
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FGuid Value;

    bool IsValid() const { return Value.IsValid(); }
    static FProximaID NewId();
};

USTRUCT(BlueprintType)
struct FProximaPropertyID
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;
    bool IsValid() const { return Id.IsValid(); }
};

USTRUCT(BlueprintType)
struct FProximaBuildingID
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;
    bool IsValid() const { return Id.IsValid(); }
};

USTRUCT(BlueprintType)
struct FProximaFloorID
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;
    bool IsValid() const { return Id.IsValid(); }
};

USTRUCT(BlueprintType)
struct FProximaWallID
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;
    bool IsValid() const { return Id.IsValid(); }
};

USTRUCT(BlueprintType)
struct FProximaOpeningID
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;
    bool IsValid() const { return Id.IsValid(); }
};

/** Stable catalog key, intended to be namespaced (for example Proxima.Wall.Paint.White). */
USTRUCT(BlueprintType)
struct FProximaCatalogID
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FName Value = NAME_None;
    bool IsValid() const { return Value != NAME_None; }
};
