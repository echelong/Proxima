# Foundation Review Handoff

## Status
The autonomous scaffold has received a static architecture/code review, but it still has **not** been compiled with Unreal Engine. Do not describe the foundation as compile-verified until UnrealBuildTool, UnrealHeaderTool, the C++ compiler, and automation tests have actually run.

## Important repairs made in the review patch
- Removed invalid/misleading GameMode redirects and nonexistent default map references.
- Removed an unsupported/suspicious target-rule setting (`bCompileWithAdminCode`).
- Switched the game module to the primary-game-module macro and declared `LogProxima`.
- Corrected generated-header/include mistakes and explicit includes.
- Corrected Unreal coordinate semantics: walls use horizontal XY, Z is elevation, and centimeters are not scaled by `0.01` when converted to world coordinates.
- Replaced process-local integer IDs with GUID-backed persistent IDs.
- Removed the unsafe global `UProximaBuildingManager::Get()` world lookup.
- Reworked building-model APIs to avoid Blueprint-exposed pointers/references to USTRUCT storage.
- Made build commands report success/failure and made undo/redo history transactional.
- Replaced redundant Live/Build UObject mode shells with one interaction-mode enum/state.
- Changed save data from `UDataAsset` to `USaveGame` and reset the first real save format to V1.
- Added the missing save-version utility implementation and save validation.
- Changed snapping into a stateless Blueprint function library instead of constructing UObjects on the stack.
- Added explicit metric parsing for `m`, `cm`, and decimal comma input.
- Corrected snapping midpoint test expectations.
- Removed a command test that instantiated an abstract command and did not test undo/redo.
- Updated docs so they no longer claim compilation or tests succeeded.

## Still unverified
- UnrealHeaderTool compatibility of every reflected signature.
- Unreal Engine 5.5 C++ compilation on Linux.
- Runtime subsystem initialization.
- Automation test execution.
- Enhanced Input assets and mapping contexts.
- Runtime wall Actor/mesh reconstruction.
- Build camera and wall placement tool.

## Highest-value next tasks
1. Compile the Editor target with the exact installed Unreal Engine version.
2. Fix all UHT/compiler diagnostics without adding gameplay features.
3. Run `Proxima.*` automation tests.
4. Add a GameInstance-backed command undo/redo test.
5. Only after the foundation is green, implement the wall-placement session and preview Actor.

## Manual-review notes
- `UProximaBuildingManager` is intentionally limited to model state for now. If it starts accumulating Actor spawning, mesh generation, input, save I/O, or catalog behavior, split those responsibilities immediately.
- Explicit wall connectivity is provisional. Room detection should eventually use a validated planar topology/graph rather than trusting stale neighbor arrays.
- Catalog UGC should be introduced through a registry/source layer, not by making persistent wall data reference runtime imported objects.
