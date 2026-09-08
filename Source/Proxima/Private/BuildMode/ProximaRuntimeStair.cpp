#include "BuildMode/ProximaRuntimeStair.h"

#include "BuildMode/ProximaMaterials.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AProximaRuntimeStair::AProximaRuntimeStair()
{
    PrimaryActorTick.bCanEverTick =
        false;

    Steps =
        CreateDefaultSubobject<
            UInstancedStaticMeshComponent>(
                TEXT("StairSteps"));

    RootComponent =
        Steps;

    Steps->SetMobility(
        EComponentMobility::Movable);

    Steps->SetCollisionProfileName(
        TEXT("BlockAll"));

    /*
     * Do not rely only on the profile default here.
     * The character must physically walk each generated stair tread.
     */
    Steps->SetCollisionEnabled(
        ECollisionEnabled::QueryAndPhysics);

    Steps->SetCollisionResponseToAllChannels(
        ECollisionResponse::ECR_Block);

    Steps->SetGenerateOverlapEvents(
        false);

    Steps->SetCanEverAffectNavigation(
        true);

    static ConstructorHelpers::
        FObjectFinder<UStaticMesh>
            Cube(
                TEXT(
                    "/Engine/BasicShapes/"
                    "Cube.Cube"));

    static ConstructorHelpers::
        FObjectFinder<UMaterialInterface>
            Basic(
                TEXT(
                    "/Engine/BasicShapes/"
                    "BasicShapeMaterial."
                    "BasicShapeMaterial"));

    if (Cube.Succeeded())
    {
        Steps->SetStaticMesh(
            Cube.Object);
    }

    if (Basic.Succeeded())
    {
        Steps->SetMaterial(
            0,
            Basic.Object);
    }
}

bool AProximaRuntimeStair::InitializeFromData(
    const FProximaStairData& Data,
    float LowerElevationCm,
    float UpperElevationCm)
{
    Steps->ClearInstances();

    if (!Data.IsValid() ||
        !FMath::IsFinite(
            LowerElevationCm) ||
        !FMath::IsFinite(
            UpperElevationCm) ||
        UpperElevationCm <=
            LowerElevationCm +
            100.0f)
    {
        return false;
    }

    StairId =
        Data.StairId;

    const float RiseCm =
        UpperElevationCm -
        LowerElevationCm;

    const float RunCm =
        Data.GetRunCm();

    const FVector2D Direction =
        (
            Data.EndCm -
            Data.StartCm
        ).GetSafeNormal();

    if (Direction.IsNearlyZero())
    {
        return false;
    }

    /*
     * Keep risers near or below 18 cm.
     */
    const int32 RiserCount =
        FMath::Clamp(
            FMath::CeilToInt(
                RiseCm /
                18.0f),
            2,
            64);

    const float RiserHeightCm =
        RiseCm /
        static_cast<float>(
            RiserCount);

    const float TreadDepthCm =
        RunCm /
        static_cast<float>(
            RiserCount);

    const float YawDegrees =
        FMath::RadiansToDegrees(
            FMath::Atan2(
                Direction.Y,
                Direction.X));

    for (int32 Index = 0;
         Index < RiserCount;
         ++Index)
    {
        const float StepTopCm =
            RiserHeightCm *
            static_cast<float>(
                Index + 1);

        const FVector2D StepCenter =
            Data.StartCm +
            Direction *
                (
                    TreadDepthCm *
                    (
                        static_cast<float>(
                            Index) +
                        0.5f
                    )
                );

        Steps->AddInstance(
            FTransform(
                FRotator(
                    0.0f,
                    YawDegrees,
                    0.0f),
                FVector(
                    StepCenter.X,
                    StepCenter.Y,
                    LowerElevationCm +
                        StepTopCm *
                        0.5f),
                FVector(
                    TreadDepthCm,
                    Data.WidthCm,
                    StepTopCm) /
                    100.0f));
    }

    BaseTint =
        FLinearColor(
            0.33f,
            0.24f,
            0.16f);

    Surface =
        FProximaMaterials::Create(
            this,
            Steps->GetMaterial(0),
            BaseTint,
            0.5f);

    if (Surface)
    {
        Steps->SetMaterial(
            0,
            Surface);
    }

    return
        Steps->GetInstanceCount() ==
        RiserCount;
}

void AProximaRuntimeStair::SetSelected(
    bool bSelected)
{
    if (!Surface)
    {
        return;
    }

    Surface->SetVectorParameterValue(
        TEXT("Tint"),
        bSelected
            ? FLinearColor(
                0.1f,
                0.65f,
                0.53f)
            : BaseTint);
}
