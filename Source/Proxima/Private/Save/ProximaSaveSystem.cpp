#include "Save/ProximaSaveSystem.h"
#include "Building/ProximaBuildingManager.h"
#include "Save/ProximaSaveData.h"
#include "Save/ProximaSerializationUtility.h"
#include "ProximaModule.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const FName ProximaSaveFormat(TEXT("ProximaSave"));
}

void UProximaSaveSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

bool UProximaSaveSystem::SaveProperty(const FString& SlotName, UProximaSaveData* Data)
{
    if (Data == nullptr || SlotName.TrimStartAndEnd().IsEmpty() ||
        !UProximaBuildingManager::ValidateModel(Data->Walls, Data->Slabs))
    {
        return false;
    }

    Data->Header.Format = ProximaSaveFormat;
    Data->Header.Version = UProximaSerializationUtility::GetSaveFormatVersion();
    Data->Header.SaveTimeUtc = FDateTime::UtcNow();

    const bool bSuccess = UGameplayStatics::SaveGameToSlot(Data, ToNativeSlotName(SlotName), 0);
    if (!bSuccess)
    {
        UE_LOG(LogProxima, Error, TEXT("Failed to save property slot '%s'."), *SlotName);
    }
    return bSuccess;
}

bool UProximaSaveSystem::LoadProperty(const FString& SlotName, UProximaSaveData*& OutData)
{
    OutData = nullptr;
    if (SlotName.TrimStartAndEnd().IsEmpty())
    {
        return false;
    }

    USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(ToNativeSlotName(SlotName), 0);
    UProximaSaveData* ProximaData = Cast<UProximaSaveData>(Loaded);
    if (ProximaData == nullptr || ProximaData->Header.Format != ProximaSaveFormat ||
        !UProximaSerializationUtility::ValidateSaveVersion(ProximaData->Header.Version) ||
        !UProximaBuildingManager::ValidateModel(ProximaData->Walls, ProximaData->Slabs))
    {
        UE_LOG(LogProxima, Warning, TEXT("Rejected invalid or unsupported Proxima save slot '%s'."), *SlotName);
        return false;
    }

    OutData = ProximaData;
    return true;
}

bool UProximaSaveSystem::IsSlotValid(const FString& SlotName) const
{
    return !SlotName.TrimStartAndEnd().IsEmpty() &&
        UGameplayStatics::DoesSaveGameExist(ToNativeSlotName(SlotName), 0);
}


bool UProximaSaveSystem::DeleteProperty(const FString& SlotName)
{
    if (SlotName.TrimStartAndEnd().IsEmpty())
    {
        return false;
    }

    const FString NativeSlot = ToNativeSlotName(SlotName);
    if (!UGameplayStatics::DoesSaveGameExist(NativeSlot, 0))
    {
        return true;
    }
    return UGameplayStatics::DeleteGameInSlot(NativeSlot, 0);
}

FString UProximaSaveSystem::ToNativeSlotName(const FString& SlotName)
{
    return TEXT("Proxima_") + SlotName.TrimStartAndEnd();
}
