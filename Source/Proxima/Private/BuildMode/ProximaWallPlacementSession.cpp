#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallSnapping.h"
#include "Building/ProximaBuildingManager.h"

void UProximaWallPlacementSession::BeginPlacement()
{
    CurrentState = EProximaPlacementState::ChoosingStart;
    StartPointCm = FVector2D::ZeroVector;
    CurrentEndpointCm = FVector2D::ZeroVector;
    SnappedEndpointCm = FVector2D::ZeroVector;
    bCanConfirm = false;
}

void UProximaWallPlacementSession::CancelPlacement()
{
    CurrentState = EProximaPlacementState::Inactive;
    bCanConfirm = false;
}

void UProximaWallPlacementSession::ConfirmStart(const FVector2D& StartCm)
{
    StartPointCm = StartCm;
    CurrentEndpointCm = StartCm;
    SnappedEndpointCm = StartCm;
    bCanConfirm = false;
    CurrentState = EProximaPlacementState::Previewing;
}

void UProximaWallPlacementSession::UpdateEndpoint(const FVector2D& CandidateCm, const FVector2D& SnappedCm)
{
    // Reject effectively-zero walls.
    if (FVector2D::Distance(StartPointCm, SnappedCm) < FMath::Max(MinWallLengthCm, 1.0f))
    {
        bCanConfirm = false;
        CurrentEndpointCm = CandidateCm;
        SnappedEndpointCm = SnappedCm;
        PreviewLengthM = 0.0f;
        return;
    }

    // Reject duplicate walls (same endpoints, same orientation) — only when a world exists.
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UProximaBuildingManager* Manager = Cast<UProximaBuildingManager>(GI->GetSubsystem<UProximaBuildingManager>()))
            {
                for (const FProximaWallData& W : Manager->GetAllWalls())
                {
                    if (W.IsDegenerate())
                    {
                        continue;
                    }
                    const FVector2D WStart = W.StartPoint.ToVector2D();
                    const FVector2D WEnd = W.EndPoint.ToVector2D();
                    const float Tol = 5.0f;
                    // Same wall in either direction (start→end or end→start).
                    bool bMatchesForward = FVector2D::Distance(WStart, SnappedCm) < Tol &&
                                          FVector2D::Distance(WEnd, StartPointCm) < Tol;
                    bool bMatchesReverse = FVector2D::Distance(WEnd, SnappedCm) < Tol &&
                                          FVector2D::Distance(WStart, StartPointCm) < Tol;
                    if (bMatchesForward || bMatchesReverse)
                    {
                        bCanConfirm = false;
                        PreviewLengthM = FVector2D::Distance(StartPointCm, SnappedCm) / 100.0f;
                        CurrentEndpointCm = CandidateCm;
                        SnappedEndpointCm = SnappedCm;
                        return;
                    }
                }
            }
        }
    }

    CurrentEndpointCm = CandidateCm;
    SnappedEndpointCm = SnappedCm;
    PreviewLengthM = FVector2D::Distance(StartPointCm, SnappedCm) / 100.0f;
    bCanConfirm = CurrentState == EProximaPlacementState::Previewing;
}

FVector2D UProximaWallPlacementSession::SnapEndpoint(
    const FVector2D& CandidateCm,
    const TArray<FVector2D>& ExistingEndpointsCm,
    float EndpointToleranceCm) const
{
    FVector2D Endpoint;
    if (UProximaWallSnapping::FindNearestEndpoint(CandidateCm, ExistingEndpointsCm, EndpointToleranceCm, Endpoint))
    {
        return Endpoint;
    }

    return UProximaWallSnapping::SnapPointToGrid(CandidateCm, GridSnapCm);
}
