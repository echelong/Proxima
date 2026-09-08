#include "BuildMode/ProximaBuildCamera.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"

AProximaBuildCamera::AProximaBuildCamera()
{
    PrimaryActorTick.bCanEverTick = false;

    Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
    RootComponent = Pivot;

    Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
    Arm->SetupAttachment(RootComponent);
    Arm->TargetArmLength = DefaultDistance;
    Arm->bUsePawnControlRotation = false;
    Arm->bDoCollisionTest = false;
    Arm->bEnableCameraLag = true;
    Arm->CameraLagSpeed = 6.0f;
    PitchDegrees = ClampPitch(
        DefaultPitch,
        MinPitch,
        MaxPitch);
    Arm->SetRelativeRotation(
        FRotator(PitchDegrees, 0.0f, 0.0f));

    Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
    Cam->SetupAttachment(Arm, USpringArmComponent::SocketName);
    Cam->bUsePawnControlRotation = false;
}

FVector AProximaBuildCamera::HorizontalForwardFromYaw(float InYawDegrees)
{
    return FRotationMatrix(FRotator(0.0f, InYawDegrees, 0.0f)).GetUnitAxis(EAxis::X);
}

FVector AProximaBuildCamera::HorizontalRightFromYaw(float InYawDegrees)
{
    return FRotationMatrix(FRotator(0.0f, InYawDegrees, 0.0f)).GetUnitAxis(EAxis::Y);
}

float AProximaBuildCamera::NormalizeYaw(float InYawDegrees)
{
    float Result = FMath::Fmod(InYawDegrees, 360.0f);
    if (Result < 0.0f)
    {
        Result += 360.0f;
    }
    return Result;
}

float AProximaBuildCamera::ClampPitch(
    float PitchDegrees,
    float MinPitchDegrees,
    float MaxPitchDegrees)
{
    const float Lower =
        FMath::Min(
            MinPitchDegrees,
            MaxPitchDegrees);

    const float Upper =
        FMath::Max(
            MinPitchDegrees,
            MaxPitchDegrees);

    return FMath::Clamp(
        PitchDegrees,
        Lower,
        Upper);
}

float AProximaBuildCamera::ClampZoomDistance(float DistanceCm, float MinDistanceCm, float MaxDistanceCm)
{
    return FMath::Clamp(DistanceCm, MinDistanceCm, MaxDistanceCm);
}

FVector AProximaBuildCamera::GetForward() const
{
    return HorizontalForwardFromYaw(YawDegrees);
}

FVector AProximaBuildCamera::GetRight() const
{
    return HorizontalRightFromYaw(YawDegrees);
}

void AProximaBuildCamera::SetYaw(float NewYaw)
{
    YawDegrees = NormalizeYaw(NewYaw);
    UpdateCameraTransform();
}

void AProximaBuildCamera::SetPitch(float NewPitch)
{
    PitchDegrees = ClampPitch(
        NewPitch,
        MinPitch,
        MaxPitch);

    UpdateCameraTransform();
}

void AProximaBuildCamera::Pan(const FVector2D& DeltaCm)
{
    if (!Pivot)
    {
        return;
    }

    const FVector Move = GetForward() * DeltaCm.X + GetRight() * DeltaCm.Y;
    Pivot->AddWorldOffset(Move);
}

void AProximaBuildCamera::RotateYaw(float DeltaDegrees)
{
    SetYaw(YawDegrees + DeltaDegrees);
}

void AProximaBuildCamera::RotatePitch(float DeltaDegrees)
{
    SetPitch(PitchDegrees + DeltaDegrees);
}

void AProximaBuildCamera::Zoom(float DeltaCm)
{
    if (!Arm)
    {
        return;
    }

    Arm->TargetArmLength = ClampZoomDistance(
        Arm->TargetArmLength + DeltaCm,
        MinZoom,
        MaxZoom);
}

void AProximaBuildCamera::InitializeOverPoint(const FVector& WorldPoint)
{
    if (Pivot)
    {
        Pivot->SetWorldLocation(WorldPoint);
    }

    YawDegrees = 0.0f;
    PitchDegrees = ClampPitch(
        DefaultPitch,
        MinPitch,
        MaxPitch);

    if (Arm)
    {
        Arm->TargetArmLength = ClampZoomDistance(DefaultDistance, MinZoom, MaxZoom);
    }
    UpdateCameraTransform();
}

void AProximaBuildCamera::UpdateCameraTransform()
{
    SetActorRotation(FRotator(0.0f, YawDegrees, 0.0f));
    if (Arm)
    {
        Arm->SetRelativeRotation(
            FRotator(PitchDegrees, 0.0f, 0.0f));
    }
}
