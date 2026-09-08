#include "BuildMode/ProximaRuntimeWall.h"
#include "BuildMode/ProximaMaterials.h"
#include "Building/ProximaGeometryKernel.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
FTransform Box(double X, double Z, double Width, double Depth, double Height)
{
    return FTransform(FRotator::ZeroRotator, FVector(X, 0.0, Z), FVector(Width, Depth, Height) / 100.0);
}
}

AProximaRuntimeWall::AProximaRuntimeWall()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;
    Solids = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Solids"));
    Frames = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Frames"));
    Glass = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Glass"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    for (UInstancedStaticMeshComponent* Component : {Solids.Get(), Frames.Get(), Glass.Get()})
    {
        Component->SetupAttachment(SceneRoot);
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetCollisionProfileName(TEXT("BlockAll"));
        Component->SetGenerateOverlapEvents(false);
        if (Cube.Succeeded()) { Component->SetStaticMesh(Cube.Object); }
        if (Basic.Succeeded()) { Component->SetMaterial(0, Basic.Object); }
    }
    Glass->SetCastShadow(false);
}

void AProximaRuntimeWall::InitializeFromData(
    const FProximaWallData& Data,
    float OriginX,
    float OriginY,
    float BaseElevationCm)
{
    Solids->ClearInstances(); Frames->ClearInstances(); Glass->ClearInstances();
    WallId = Data.WallId;
    if (!Data.IsValid()) { return; }
    SetActorTransform(
        FTransform(
            Data.GetRotation(),
            FVector(
                OriginX + Data.StartPoint.XCm,
                OriginY + Data.StartPoint.YCm,
                BaseElevationCm)));
    std::vector<ProximaGeometry::Rect> Openings, Boxes;
    for (const FProximaOpeningData& O : Data.Openings)
    {
        Openings.push_back({O.OffsetFromStartCm, O.BottomHeightCm,
            static_cast<double>(O.OffsetFromStartCm) + O.WidthCm, O.TopHeightCm});
    }
    if (!ProximaGeometry::WallSolids(Data.GetLengthCm(), Data.HeightCm, Openings, Boxes)) { return; }
    for (const auto& R : Boxes)
    {
        Solids->AddInstance(Box((R.Left + R.Right) * 0.5, (R.Bottom + R.Top) * 0.5,
            R.Right - R.Left, Data.ThicknessCm, R.Top - R.Bottom));
    }
    for (const FProximaOpeningData& O : Data.Openings)
    {
        const double Left = O.OffsetFromStartCm, Right = Left + O.WidthCm;
        const double Bottom = O.BottomHeightCm, Top = O.TopHeightCm, MidX = (Left + Right) * 0.5;
        // Frames sit outside the aperture, preserving the clear opening width.
        const double FrameDepth = Data.ThicknessCm + 2.0;
        Frames->AddInstance(Box(Left - 2.0, (Bottom + Top) * 0.5, 4.0, FrameDepth, Top - Bottom));
        Frames->AddInstance(Box(Right + 2.0, (Bottom + Top) * 0.5, 4.0, FrameDepth, Top - Bottom));
        Frames->AddInstance(Box(MidX, Top + 2.0, O.WidthCm + 8.0, FrameDepth, 4.0));
        if (Bottom > 0.0) { Frames->AddInstance(Box(MidX, Bottom - 2.0, O.WidthCm + 8.0, FrameDepth, 4.0)); }
        if (O.Type == EProximaOpeningType::Window)
        {
            Glass->AddInstance(Box(MidX, (Bottom + Top) * 0.5, O.WidthCm, 1.0, Top - Bottom));
        }
    }
    BaseTint = FProximaMaterials::WallTint(Data.SideAMaterial.Value);
    Surface = FProximaMaterials::Create(this, Solids->GetMaterial(0), BaseTint);
    if (Surface) { Solids->SetMaterial(0, Surface); }
    Frames->SetMaterial(0, FProximaMaterials::Create(this, Frames->GetMaterial(0), FLinearColor(0.07f, 0.085f, 0.08f), 0.45f));
    Glass->SetMaterial(0, FProximaMaterials::Create(this, Glass->GetMaterial(0), FLinearColor(0.65f, 0.85f, 0.9f), 0.12f, true));
}
void AProximaRuntimeWall::SetSelected(bool bSelected)
{
    if (Surface) { Surface->SetVectorParameterValue(TEXT("Tint"), bSelected ? FLinearColor(0.1f, 0.65f, 0.53f) : BaseTint); }
}
int32 AProximaRuntimeWall::GetSolidCount() const { return Solids->GetInstanceCount(); }

void AProximaRuntimeWall::AddCornerPatch(const FVector2D& Min, const FVector2D& Max, float HeightCm)
{
    const FVector2D Center = (Min + Max) * 0.5;
    Solids->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Center.X, Center.Y, HeightCm * 0.5f),
        FVector(Max.X - Min.X, Max.Y - Min.Y, HeightCm) / 100.0));
}
