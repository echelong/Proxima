#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "ProximaCharacter.generated.h"

UCLASS()
class PROXIMA_API AProximaCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AProximaCharacter();

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|Live", BlueprintReadOnly)
    float WalkSpeed = 180.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|Live", BlueprintReadOnly)
    float SprintSpeed = 450.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|Live", BlueprintReadOnly)
    float LiveCameraDistance = 700.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|Live", BlueprintReadOnly)
    float LiveCameraPitchMin = -30.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Proxima|Live", BlueprintReadOnly)
    float LiveCameraPitchMax = 45.0f;

    UFUNCTION(BlueprintPure, Category = "Proxima|Live")
    bool IsSprinting() const { return bSprintRequested; }

    UFUNCTION(BlueprintPure, Category = "Proxima|Live")
    USpringArmComponent* GetSpringArmComponent() const { return SpringArm; }

    UFUNCTION(BlueprintCallable, Category = "Proxima|Live")
    void StartSprintBP() { StartSprint(); }

    UFUNCTION(BlueprintCallable, Category = "Proxima|Live")
    void StopSprintBP() { StopSprint(); }

    /** Live-mode forward movement called by the controller-owned input router. */
    void MoveForward(float Value);

    /** Live-mode right movement called by the controller-owned input router. */
    void MoveRight(float Value);
    void ToggleInspectionView();

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proxima|Live")
    TObjectPtr<USpringArmComponent> SpringArm = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proxima|Live")
    TObjectPtr<UCameraComponent> Camera = nullptr;

private:
    bool bSprintRequested = false;
    bool bInspectionView = true;

    void StartSprint();
    void StopSprint();
};
