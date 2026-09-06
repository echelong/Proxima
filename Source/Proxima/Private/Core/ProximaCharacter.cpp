#include "Core/ProximaCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"

AProximaCharacter::AProximaCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->MaxWalkSpeedCrouched = WalkSpeed * 0.5f;
    Movement->BrakingDecelerationWalking = 2048.0f;
    Movement->JumpZVelocity = 620.0f;
    Movement->AirControl = 0.35f;
    Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
    // Character yaw follows control yaw — camera-relative strafing.
    Movement->bOrientRotationToMovement = true;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = LiveCameraDistance;
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bInheritPitch = true;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritRoll = false;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 8.0f;
    SpringArm->bEnableCameraRotationLag = true;
    SpringArm->CameraRotationLagSpeed = 8.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
}

void AProximaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        return;
    }

    // Single authoritative movement architecture: Character owns WASD via axis bindings.
    // W=+1.0, S=-1.0 → MoveForward.  D=+1.0, A=-1.0 → MoveRight.
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AProximaCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"),   this, &AProximaCharacter::MoveRight);
}

void AProximaCharacter::MoveForward(float Value)
{
    if (Value == 0.0f)
    {
        return;
    }

    // Camera-relative movement: forward is control yaw direction.
    const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AProximaCharacter::MoveRight(float Value)
{
    if (Value == 0.0f)
    {
        return;
    }

    const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void AProximaCharacter::StartSprint()
{
    bSprintRequested = true;
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = SprintSpeed;
    }
}

void AProximaCharacter::StopSprint()
{
    bSprintRequested = false;
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = WalkSpeed;
    }
}
