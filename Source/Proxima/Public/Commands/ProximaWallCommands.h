#pragma once

#include "Commands/ProximaBuildCommand.h"
#include "Building/ProximaWallData.h"
#include "ProximaWallCommands.generated.h"

UCLASS()
class PROXIMA_API UProximaCreateWallCommand : public UProximaBuildCommand
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Proxima|Commands")
    FProximaWallData WallData;

    virtual bool Execute_Implementation() override;
    virtual bool Undo_Implementation() override;
};

UCLASS()
class PROXIMA_API UProximaDeleteWallCommand : public UProximaBuildCommand
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Proxima|Commands")
    FProximaWallID WallId;

    virtual bool Execute_Implementation() override;
    virtual bool Undo_Implementation() override;

private:
    UPROPERTY(Transient)
    FProximaWallData DeletedWallData;

    bool bCapturedWall = false;
};

UCLASS()
class PROXIMA_API UProximaAddWallOpeningCommand : public UProximaBuildCommand
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Proxima|Commands")
    FProximaWallID WallId;

    UPROPERTY(BlueprintReadWrite, Category = "Proxima|Commands")
    FProximaOpeningData OpeningData;

    virtual bool Execute_Implementation() override;
    virtual bool Undo_Implementation() override;
};

UCLASS()
class PROXIMA_API UProximaModifyWallCommand : public UProximaBuildCommand
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Proxima|Commands")
    FProximaWallID WallId;

    UPROPERTY(BlueprintReadWrite, Category = "Proxima|Commands")
    FProximaWallData NewData;

    virtual bool Execute_Implementation() override;
    virtual bool Undo_Implementation() override;

private:
    UPROPERTY(Transient)
    FProximaWallData OldData;

    bool bCapturedOldData = false;
};
