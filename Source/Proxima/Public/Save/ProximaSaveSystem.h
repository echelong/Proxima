#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProximaSaveSystem.generated.h"

class UProximaSaveData;

UCLASS()
class PROXIMA_API UProximaSaveSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Proxima|Save")
    bool SaveProperty(const FString& SlotName, UProximaSaveData* Data);

    UFUNCTION(BlueprintCallable, Category = "Proxima|Save")
    bool LoadProperty(const FString& SlotName, UProximaSaveData*& OutData);

    UFUNCTION(BlueprintPure, Category = "Proxima|Save")
    bool IsSlotValid(const FString& SlotName) const;

private:
    static FString ToNativeSlotName(const FString& SlotName);
};
