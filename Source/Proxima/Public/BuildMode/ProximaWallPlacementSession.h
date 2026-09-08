#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProximaWallPlacementSession.generated.h"

UENUM(BlueprintType)
enum class EProximaPlacementState : uint8
{
    Inactive,
    ChoosingStart,
    Previewing,
};

UCLASS(BlueprintType)
class PROXIMA_API UProximaWallPlacementSession : public UObject
{
    GENERATED_BODY()

public:
    void BeginPlacement();
    void CancelPlacement();
    void ConfirmStart(const FVector2D& StartCm);
    void ContinueFromCurrentEndpoint();

    /**
     * During A->B->C->D rectangle drawing, constrains the third wall C->D
     * along the reverse direction of A->B and prevents it exceeding A->B.
     */
    bool TryResolveRectangleThirdWall(
        const FVector2D& CandidateCm,
        FVector2D& OutEndpointCm,
        bool& bOutAtLimit,
        float DirectionDotThreshold = 0.98f) const;

    /**
     * Gives a nearly closed wall chain a stronger snap back to its first
     * point. Used after three or more completed walls.
     */
    bool TrySnapToChainStart(
        const FVector2D& CandidateCm,
        float ToleranceCm,
        FVector2D& OutEndpointCm) const;

    void UpdateEndpoint(const FVector2D& CandidateCm, const FVector2D& SnappedCm, bool bDuplicateGeometry = false);

    bool IsActive() const { return CurrentState != EProximaPlacementState::Inactive; }
    EProximaPlacementState GetState() const { return CurrentState; }

    UPROPERTY(BlueprintReadOnly, Category="Proxima|Placement")
    FVector2D StartPointCm = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Proxima|Placement")
    FVector2D CurrentEndpointCm = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Proxima|Placement")
    FVector2D SnappedEndpointCm = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Proxima|Placement")
    bool bCanConfirm = false;

    /**
     * Points committed during the current continuous wall-drawing gesture.
     * Example rectangle after two walls: [A, B, C].
     */
    UPROPERTY(BlueprintReadOnly, Category="Proxima|Placement")
    TArray<FVector2D> ChainPointsCm;

    const TArray<FVector2D>& GetChainPoints() const
    {
        return ChainPointsCm;
    }

    /** Current wall length in metres (centimetres → metres for display). Updated by UpdateEndpoint. */
    UPROPERTY(BlueprintReadOnly, Category="Proxima|Placement")
    float PreviewLengthM = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category="Proxima|Placement")
    float DefaultHeightCm = 270.0f;

    UPROPERTY(EditDefaultsOnly, Category="Proxima|Placement")
    float DefaultThicknessCm = 15.0f;

    UPROPERTY(EditDefaultsOnly, Category="Proxima|Placement")
    float GridSnapCm = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category="Proxima|Placement")
    float MinWallLengthCm = 1.0f;

    /** Endpoint snap takes priority over grid snap so existing corners remain exact. */
    UFUNCTION(BlueprintCallable, Category="Proxima|Placement")
    FVector2D SnapEndpoint(
        const FVector2D& CandidateCm,
        const TArray<FVector2D>& ExistingEndpointsCm,
        float EndpointToleranceCm = 15.0f) const;

private:
    EProximaPlacementState CurrentState = EProximaPlacementState::Inactive;
};
