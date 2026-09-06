# Build Mode v0.1 — Manual Review Handoff

## Status
The autonomous Build Mode implementation was compile-tested before handoff, but manual source review found several runtime correctness gaps that compilation could not detect. A manual repair pass has now been prepared. **Rebuild and rerun all `Proxima` automation tests before committing.**

Persistent `FProximaWallData` remains the source of truth. Runtime wall actors and preview actors are representations only.

## Manual Review Findings Corrected
- Build input was implemented as per-frame `IsInputKeyDown` polling but the polling function was never called. Even if called, held keys would repeatedly toggle modes, create walls, and undo/redo every frame. Input is now bound to key **Pressed** events in `SetupInputComponent`.
- `SnappedEndpointCm` was never updated, so the preview could remain zero-length while confirmation used a different endpoint. The placement session now tracks raw candidate and snapped endpoint separately.
- Zero-length walls could be marked confirmable. `MinWallLengthCm` now gates confirmation.
- Confirm/cancel previously left Build Mode active with the placement session inactive, preventing the next wall. Confirm and cancel now return to `ChoosingStart` while remaining in Build Mode.
- `SyncRebuildWallActors()` was a no-op, so command Undo/Redo did not update runtime walls. The building model now broadcasts a wall-change delegate, and the player controller rebuilds runtime representations from persistent data on every successful wall mutation.
- Runtime walls were centered at floor Z rather than half their wall height above the floor, placing half the wall below the build plane. Preview and runtime actors now share one pure wall transform utility.
- The build-plane helper divided by ray Z before checking for a near-parallel ray and did not perform the promised geometry trace. It now performs a visibility trace first, projects the hit XY to the active build plane, and safely falls back to ray/plane intersection.
- Endpoint snapping now takes priority over grid snapping so an existing endpoint that is not itself on the current grid remains exact.
- The duplicate/misleading `ResolveExactLengthEndpoint` helper in wall snapping was removed. `UProximaExactLength` remains the single exact-length API.
- The autonomous scratch file `Private/BuildPlan.txt` was removed.
- The generated Android file-server security token was removed from tracked config.

## Current Build Mode Architecture

Input event
→ `AProximaPlayerController`
→ controller-owned `UProximaWallPlacementSession`
→ cursor/build-plane resolution
→ endpoint/grid snapping
→ preview actor
→ `UProximaCreateWallCommand`
→ `UProximaBuildingManager` persistent wall data
→ wall-model-changed delegate
→ deterministic runtime wall reconstruction

Undo/Redo follows the same persistent mutation path, so runtime actors are rebuilt from the resulting model state rather than treated as authoritative.

## Controls
- **B**: Live ↔ Build Mode
- **Left Mouse**: set start / confirm current wall
- **Right Mouse** or **Escape**: cancel current wall and stay ready to place another
- **Ctrl+Z**: undo last build command while in Build Mode
- **Ctrl+Y**: redo last build command while in Build Mode

These controls are bound directly in C++ and do not require binary input mapping assets for v0.1.

## Placement Defaults
- Build plane: Z = 0 cm, configurable on the player controller
- Grid snap: 10 cm
- Endpoint tolerance: 15 cm
- Wall height: 270 cm
- Wall thickness: 15 cm
- Minimum confirmable wall length: 1 cm

## Geometry
`UProximaWallGeometry::MakeWallCubeTransform` is now shared by preview and runtime walls.

For the engine BasicShapes cube (100 × 100 × 100 cm):
- X scale = wall length / 100
- Y scale = wall thickness / 100
- Z scale = wall height / 100
- location = horizontal midpoint + half wall height on Z
- yaw = `atan2(DeltaY, DeltaX)`

World positions remain in centimeters. There is no `0.01` conversion applied to `FVector` positions.

## Runtime Synchronization
`UProximaBuildingManager` owns only persistent wall data. It broadcasts `OnWallsChanged()` after successful add/remove/update/reset operations. `AProximaPlayerController` subscribes during `BeginPlay`, rebuilds runtime wall actors from persistent data, and unsubscribes during `EndPlay`.

The full rebuild is intentionally simple for v0.1. Incremental actor updates can be added after correctness and UX are proven.

## Tests Added / Expanded
`WallPlacementTest.cpp` now covers:
- exact-length endpoint resolution
- negative length clamping
- grid snapping
- endpoint snapping
- endpoint priority over grid
- placement-session raw vs snapped endpoint state
- degenerate-wall confirmation rejection
- 300 × 15 × 270 cm cube transform scaling
- wall center Z placement
- X/Y wall yaw
- existing wall data world-position/rotation math

## Verification Required Before Commit
Run the UE 5.8.2 Linux editor build and then all tests under `Automation RunTests Proxima`.

Do not claim this repair is complete until both succeed locally.

## Manual Editor Review After Green Build/Tests
1. B toggles Build Mode exactly once per key press.
2. Mouse cursor targets the intended build plane.
3. First click creates a snapped start point.
4. Preview follows the mouse and has the expected 270 cm height above the floor.
5. Second click creates a visible wall of the same dimensions as the preview.
6. Another wall can be started immediately without toggling Build Mode off/on.
7. Right click/Escape cancels only the in-progress wall.
8. Ctrl+Z visibly removes the wall and its persistent data.
9. Ctrl+Y visibly restores the same wall ID/data and representation.
10. Endpoint snapping feels usable and does not pull to distant corners.

## Deferred Scope
Still intentionally excluded from v0.1: polished build UI, typed-length widget, rooms, floors, roofs, stairs, openings, curved walls, segment splitting, multiple storeys, furniture, landscaping, multiplayer, vehicles, and external/AI-generated assets.
