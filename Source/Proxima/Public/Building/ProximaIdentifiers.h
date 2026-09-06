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

inline uint32 GetTypeHash(const FProximaID& Id)
{
    return GetTypeHash(Id.Value);
}

inline bool operator==(const FProximaID& A, const FProximaID& B)
{
    return A.Value == B.Value;
}

/** Building identifier */
USTRUCT(BlueprintType)
struct FProximaBuildingID
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;

    bool IsValid() const { return Id.IsValid(); }
};

inline bool operator==(const FProximaBuildingID& A, const FProximaBuildingID& B)
{
    return A.Id == B.Id;
}

inline uint32 GetTypeHash(const FProximaBuildingID& Id)
{
    return GetTypeHash(Id.Id);
}

/** Floor identifier */
USTRUCT(BlueprintType)
struct FProximaFloorID
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;

    bool IsValid() const { return Id.IsValid(); }
};

inline bool operator==(const FProximaFloorID& A, const FProximaFloorID& B)
{
    return A.Id == B.Id;
}

inline uint32 GetTypeHash(const FProximaFloorID& Id)
{
    return GetTypeHash(Id.Id);
}

/** Wall identifier */
USTRUCT(BlueprintType)
struct FProximaWallID
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;

    bool IsValid() const { return Id.IsValid(); }
};

inline bool operator==(const FProximaWallID& A, const FProximaWallID& B)
{
    return A.Id == B.Id;
}

inline uint32 GetTypeHash(const FProximaWallID& Id)
{
    return GetTypeHash(Id.Id);
}

/** Opening identifier */
USTRUCT(BlueprintType)
struct FProximaOpeningID
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;

    bool IsValid() const { return Id.IsValid(); }
};

inline bool operator==(const FProximaOpeningID& A, const FProximaOpeningID& B)
{
    return A.Id == B.Id;
}

inline uint32 GetTypeHash(const FProximaOpeningID& Id)
{
    return GetTypeHash(Id.Id);
}

/** Property identifier */
USTRUCT(BlueprintType)
struct FProximaPropertyID
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FProximaID Id;

    bool IsValid() const { return Id.IsValid(); }
};

inline bool operator==(const FProximaPropertyID& A, const FProximaPropertyID& B)
{
    return A.Id == B.Id;
}

inline uint32 GetTypeHash(const FProximaPropertyID& Id)
{
    return GetTypeHash(Id.Id);
}

/** Stable catalog key, intended to be namespaced (for example Proxima.Wall.Paint.White). */
USTRUCT(BlueprintType)
struct FProximaCatalogID
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FName Value = NAME_None;

    bool IsValid() const { return Value != NAME_None; }
};

inline bool operator==(const FProximaCatalogID& A, const FProximaCatalogID& B)
{
    return A.Value == B.Value;
}

inline uint32 GetTypeHash(const FProximaCatalogID& Id)
{
    return GetTypeHash(Id.Value);
}