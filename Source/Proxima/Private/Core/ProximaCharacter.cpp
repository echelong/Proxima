#include "Core/ProximaCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Interaction/ProximaInteractionSubsystem.h"

AProximaCharacter::AProximaCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->MaxWalkSpeedCrouched = WalkSpeed * 0.5f;

    // Proxima uses responsive direct movement rather than a floaty
    // action-game acceleration curve.
    Movement->MaxAcceleration = 12000.0f;
    Movement->BrakingDecelerationWalking = 12000.0f;
    Movement->GroundFriction = 12.0f;
    Movement->JumpZVelocity = 620.0f;
    Movement->AirControl = 0.35f;
    Movement->RotationRate = FRotator(0.f, 1080.f, 0.f);
    // Character rotates toward movement direction; movement vectors are camera/control-yaw relative.
    Movement->bOrientRotationToMovement = true;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    GetCapsuleComponent()->InitCapsuleSize(30.0f, 90.0f);
    SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
    SpringArm->TargetArmLength = 0.0f;
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bInheritPitch = true;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritRoll = false;
    SpringArm->bEnableCameraLag = false;
    SpringArm->CameraLagSpeed = 20.0f;
    SpringArm->bEnableCameraRotationLag = false;
    SpringArm->CameraRotationLagSpeed = 20.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
}

void AProximaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // AProximaPlayerController owns all gameplay bindings and routes the legacy
    // MoveForward/MoveRight axes by interaction mode. Keep the pawn binding-free.
}

void AProximaCharacter::MoveForward(float Value)
{
    if (Value == 0.0f)
    {
        return;
    }

    // Never allow character movement during Build Mode.
    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (UProximaInteractionSubsystem* Sub = GI->GetSubsystem<UProximaInteractionSubsystem>())
        {
            if (Sub->IsBuildModeActive())
            {
                return;
            }
        }
    }

    const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AProximaCharacter::MoveRight(float Value)
{
    if (Value == 0.0f)
    {
        return;
    }

    // Never allow character movement during Build Mode.
    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (UProximaInteractionSubsystem* Sub = GI->GetSubsystem<UProximaInteractionSubsystem>())
        {
            if (Sub->IsBuildModeActive())
            {
                return;
            }
        }
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

void AProximaCharacter::ToggleInspectionView()
{
    bInspectionView = !bInspectionView;
    SpringArm->TargetArmLength = bInspectionView ? 0.0f : LiveCameraDistance;
}
