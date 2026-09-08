#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProximaBuildCamera.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

UCLASS()
class PROXIMA_API AProximaBuildCamera : public AActor
{
    GENERATED_BODY()

public:
    AProximaBuildCamera();

    /** Move the camera pivot on the horizontal plane. X is forward, Y is right. */
    UFUNCTION(BlueprintCallable, Category = "Proxima|BuildCamera")
    void Pan(const FVector2D& DeltaCm);

    /** Apply a yaw delta in degrees. */
    UFUNCTION(BlueprintCallable, Category = "Proxima|BuildCamera")
    void RotateYaw(float DeltaDegrees);

    /** Change camera-arm distance. Positive values pull back. */
    UFUNCTION(BlueprintCallable, Category = "Proxima|BuildCamera")
    void Zoom(float DeltaCm);

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|BuildCamera")
    float MinZoom = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|BuildCamera")
    float MaxZoom = 3000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|BuildCamera")
    float DefaultPitch = -55.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|BuildCamera")
    float DefaultDistance = 1800.0f;

    FVector GetForward() const;
    FVector GetRight() const;

    UFUNCTION(BlueprintPure, Category = "Proxima|BuildCamera")
    float GetYaw() const { return YawDegrees; }

    UFUNCTION(BlueprintCallable, Category = "Proxima|BuildCamera")
    void SetYaw(float NewYaw);

    UFUNCTION(BlueprintCallable, Category = "Proxima|BuildCamera")
    void InitializeOverPoint(const FVector& WorldPoint);

    UFUNCTION(BlueprintPure, Category = "Proxima|BuildCamera")
    USceneComponent* GetCameraPivot() const { return Pivot; }

    UFUNCTION(BlueprintPure, Category = "Proxima|BuildCamera")
    UCameraComponent* GetCamera() const { return Cam; }

    /** Pure helpers used by the runtime camera and deterministic automation tests. */
    static FVector HorizontalForwardFromYaw(float YawDegrees);
    static FVector HorizontalRightFromYaw(float YawDegrees);
    static float NormalizeYaw(float YawDegrees);
    static float ClampZoomDistance(float DistanceCm, float MinDistanceCm, float MaxDistanceCm);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Pivot = nullptr;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpringArmComponent> Arm = nullptr;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Cam = nullptr;

    float YawDegrees = 0.0f;

    void UpdateCameraTransform();
};
