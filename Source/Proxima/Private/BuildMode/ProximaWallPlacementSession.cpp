#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallSnapping.h"

void UProximaWallPlacementSession::BeginPlacement()
{
    CurrentState = EProximaPlacementState::ChoosingStart;
    StartPointCm = FVector2D::ZeroVector;
    CurrentEndpointCm = FVector2D::ZeroVector;
    SnappedEndpointCm = FVector2D::ZeroVector;
    PreviewLengthM = 0.0f;
    bCanConfirm = false;
}

void UProximaWallPlacementSession::CancelPlacement()
{
    CurrentState = EProximaPlacementState::Inactive;
    StartPointCm = FVector2D::ZeroVector;
    CurrentEndpointCm = FVector2D::ZeroVector;
    SnappedEndpointCm = FVector2D::ZeroVector;
    PreviewLengthM = 0.0f;
    bCanConfirm = false;
}

void UProximaWallPlacementSession::ConfirmStart(const FVector2D& StartCm)
{
    StartPointCm = StartCm;
    CurrentEndpointCm = StartCm;
    SnappedEndpointCm = StartCm;
    PreviewLengthM = 0.0f;
    bCanConfirm = false;
    CurrentState = EProximaPlacementState::Previewing;
}

void UProximaWallPlacementSession::ContinueFromCurrentEndpoint()
{
    const FVector2D NextStartCm = SnappedEndpointCm;
    BeginPlacement();
    ConfirmStart(NextStartCm);
}

void UProximaWallPlacementSession::UpdateEndpoint(
    const FVector2D& CandidateCm,
    const FVector2D& SnappedCm,
    bool bDuplicateGeometry)
{
    CurrentEndpointCm = CandidateCm;
    SnappedEndpointCm = SnappedCm;

    const float LengthCm = FVector2D::Distance(StartPointCm, SnappedCm);
    PreviewLengthM = LengthCm / 100.0f;

    if (LengthCm < FMath::Max(MinWallLengthCm, 1.0f) || bDuplicateGeometry)
    {
        bCanConfirm = false;
        return;
    }

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
