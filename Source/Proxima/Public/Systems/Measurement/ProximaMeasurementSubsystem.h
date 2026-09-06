#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProximaMeasurementSubsystem.generated.h"

UENUM(BlueprintType)
enum class EProximaSnapResolution : uint8
{
    None,
    OneCm,
    FiveCm,
    TenCm,
    TwentyFiveCm,
    FiftyCm,
    OneMeter
};

USTRUCT(BlueprintType)
struct FProximaMeasurementConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    EProximaSnapResolution SnapResolution = EProximaSnapResolution::FiveCm;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    bool bSnappingEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float DefaultWallHeightCm = 270.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float DefaultWallThicknessCm = 15.0f;
};

UCLASS()
class PROXIMA_API UProximaMeasurementSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    UFUNCTION(BlueprintCallable, Category = "Proxima|Measurement")
    float Snap(float ValueCm) const;
    UFUNCTION(BlueprintPure, Category = "Proxima|Measurement")
    float GetSnapResolutionCm() const;
    UFUNCTION(BlueprintCallable, Category = "Proxima|Measurement")
    void SetSnapResolution(EProximaSnapResolution Resolution);
    UFUNCTION(BlueprintPure, Category = "Proxima|Measurement")
    FProximaMeasurementConfig GetConfig() const { return Config; }
    UFUNCTION(BlueprintCallable, Category = "Proxima|Measurement")
    void UpdateConfig(const FProximaMeasurementConfig& NewConfig);
    static float CmToMeters(float Cm) { return Cm * 0.01f; }
    static float MetersToCm(float Meters) { return Meters * 100.0f; }
private:
    UPROPERTY()
    FProximaMeasurementConfig Config;
};
