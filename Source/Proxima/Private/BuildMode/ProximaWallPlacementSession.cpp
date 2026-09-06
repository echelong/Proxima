#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallSnapping.h"

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
    CurrentEndpointCm = CandidateCm;
    SnappedEndpointCm = SnappedCm;
    bCanConfirm = CurrentState == EProximaPlacementState::Previewing &&
        FVector2D::Distance(StartPointCm, SnappedEndpointCm) >= FMath::Max(MinWallLengthCm, KINDA_SMALL_NUMBER);
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
