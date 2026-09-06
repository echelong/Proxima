#include "Core/ProximaCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

AProximaCharacter::AProximaCharacter()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->BrakingDecelerationWalking = 2048.0f;
}
