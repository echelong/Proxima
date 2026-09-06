#include "BuildMode/ProximaBuildPlaneTrace.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UProximaBuildPlaneTrace::TraceBuildPlane(
    const UObject* WorldContextObject,
    FVector& OutWorldPosition,
    float PlaneZ)
{
    OutWorldPosition = FVector::ZeroVector;

    UWorld* World = GEngine
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (!World)
    {
        return false;
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
    if (!PlayerController)
    {
        return false;
    }

    FVector RayOrigin;
    FVector RayDirection;
    if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
    {
        return false;
    }

    const FVector RayEnd = RayOrigin + RayDirection * 1000000.0f;
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProximaBuildPlaneTrace), true);
    if (APawn* Pawn = PlayerController->GetPawn())
    {
        QueryParams.AddIgnoredActor(Pawn);
    }

    if (World->LineTraceSingleByChannel(Hit, RayOrigin, RayEnd, ECC_Visibility, QueryParams))
    {
        OutWorldPosition = Hit.Location;
        OutWorldPosition.Z = PlaneZ;
        return true;
    }

    if (FMath::IsNearlyZero(RayDirection.Z))
    {
        return false;
    }

    const float T = (PlaneZ - RayOrigin.Z) / RayDirection.Z;
    if (T < 0.0f)
    {
        return false;
    }

    OutWorldPosition = RayOrigin + RayDirection * T;
    OutWorldPosition.Z = PlaneZ;
    return true;
}
