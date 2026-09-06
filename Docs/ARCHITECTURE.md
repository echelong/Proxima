# Proxima Architecture

## Engine
Unreal Engine 5.5 project scaffold. C++ owns core rules and persistent models; Blueprint exposure is used at system boundaries where designer iteration is valuable.

## Primary rule: data is the truth
Persistent architectural data is independent from runtime Actors. Runtime wall meshes/Actors must be reconstructed deterministically from saved model data.

## Current game-instance subsystems
- `UProximaMeasurementSubsystem`: measurement preferences and snap resolution.
- `UProximaBuildingManager`: active in-memory building model only. It does not own input, Actors, meshes, or save files.
- `UProximaCommandManager`: execute/undo/redo history for mutations of the building model.
- `UProximaInteractionSubsystem`: local Live/Build interaction mode and selection.
- `UProximaSaveSystem`: save-slot I/O and version validation.

## Interaction mode
Live and Build are represented by `EProximaInteractionMode` rather than separate GameMode classes. `AProximaGameMode` remains the map/session GameMode. The player controller will later react to interaction-mode changes to swap input mapping contexts, cursor behavior, camera/pawn control, and placement tools.

## Building model
The current persistent types are:
- `FProximaPropertyData`
- `FProximaBuildingData`
- `FProximaFloorData`
- `FProximaWallData`
- `FProximaOpeningData`

All architectural dimensions and local coordinates are stored in centimeters. Wall endpoints live on the horizontal XY plane; Z is elevation.

## Stable IDs
Architectural entities use GUID-backed IDs so creating new entities after loading an old save cannot collide with IDs created in a previous process.

## Save format
`UProximaSaveData` derives from `USaveGame`. The first real format is V1. No migration is claimed until a second format actually exists.

## Catalog
`UProximaCatalog` is a data asset for built-in catalog metadata. Future runtime/UGC registries can merge external sources without changing persistent building geometry, which stores stable catalog IDs rather than hard Actor references.
