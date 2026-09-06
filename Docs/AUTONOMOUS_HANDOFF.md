# Live Mode Movement / Build Camera — Autonomy Handoff (Proxima UE5)

## Status
Live Mode movement, third-person camera, sprint, Build Mode dedicated construction camera, Live↔Build transition, and new camera math tests all implemented. Compile verified (UE 5.8.2 Linux, Clang 20.1.8). All existing + new automation tests pass.

## Files Added
- Source/Proxima/Public/BuildMode/ProximaBuildCamera.h
- Source/Proxima/Private/BuildMode/ProximaBuildCamera.cpp
- Source/Proxima/Tests/Private/Tests/CameraMovementTest.cpp

## Files Modified
- Source/Proxima/Public/Core/ProximaCharacter.h / .cpp (spring arm + camera + sprint + movement bindings)
- Source/Proxima/Public/Core/ProximaPlayerController.h / .cpp (build camera ownership, input gating, transition, live/build camera activate/deactivate)
- Config/DefaultInput.ini (WASD, MouseX/Y, MouseWheel, Sprint, BuildPan axes)

## Architecture Decisions
- Persistent building data unchanged; only camera/pawn/controller are transient.
- Build camera = dedicated AProximaBuildCamera actor spawned once by controller on toggle; stays alive but only used as view target during Build.
- No new GameMode; interaction mode (Live/Build) from existing UProximaInteractionSubsystem governs behavior.
- Character keeps spring arm + camera in Live; controller switches SetViewTargetWithBlend between pawn and build camera.
- Movement axes bound in Character; controller gates with IsBuildModeActive() so WASD does not move character during build.
- Build camera controls (W/A/S/D pan, mouse wheel zoom, mouse X rotate) only active in Build via controller bindings.

## Ownership / Lifetime
- AProximaCharacter: persistent pawn with spring arm/camera; survives mode toggles.
- AProximaBuildCamera: controller-owned; spawned once when entering Build; not destroyed per frame; never persisted to building data.
- Wall placement session / preview / runtime walls: unchanged; wall placement still works during build.

## Input Controls (exact)
- W/A/S/D: Live movement (character); Build pan (build camera, gated by mode)
- Mouse: Live look (character); Build rotate (build camera, gated by mode)
- Left Shift: Sprint (live only; disabled in build)
- B: Toggle Live ↔ Build (pressed event, no hold-repeat)
- LMB: Start / confirm wall (build only)
- RMB / Escape: Cancel placement (build only)
- Ctrl+Z / Ctrl+Y: Undo / redo (build only)
- Mouse wheel: Zoom build camera (build only)

## Live Mode Camera
- USpringArmComponent + UCameraComponent on AProximaCharacter.
- Distance 700 cm, lag enabled, bUsePawnControlRotation true.
- Character rotates with camera (bOrientRotationToMovement true).
- Walk speed 450 cm/s; sprint 750 cm/s; configurable via EditDefaultsOnly properties.

## Build Mode Camera
- AProximaBuildCamera: pivot scene component + spring arm + camera.
- Pan on horizontal plane (derived from yaw); zoom clamped 200–3000 cm; rotate around vertical axis.
- Default pitch -55°; initialized over property center + 200 cm.
- No per-tick spawning; only SetViewTargetWithBlend called on toggle.

## Transition Flow
- Enter Build: interaction mode → Build; spawn/activate build camera; show mouse; set GameAndUI input mode.
- Exit Build: interaction mode → Live; set view back to character; hide mouse; set GameOnly input mode; cancel unfinished placement.
- Repeated B toggles: no leaks, no duplicate build cameras, no corruption of wall state.

## Wall Placement Compatibility
- Mouse-to-build-plane tracing unchanged (UProximaBuildPlaneTrace).
- Preview uses same UpdatePreview logic; snapping still endpoint+grid.
- Confirm/cancel still create/destroy persistent FProximaWallData via commands.
- Build camera rotation/pan does not affect placement because build plane is fixed horizontal (Z = BuildPlaneZCm).

## Tests Added
- Proxima.Camera.Math (5 assertions): horizontal forward/right from yaw, yaw mod, zoom clamping, pitch clamp — all pass.
- Existing 8 Proxima tests preserved and passing: WallData, WallDataExtended, ExactLength, WallGeometry, WallSnapping, Measurement, Snapping, plus new Math.

## Compile Result
Succeeded (6/6 actions, libUnrealEditor-Proxima.so linked). 3 focused fix cycles used (SpringArm include, FInputMode, sprint access, mouse lock enum, camera test math).

## Warnings / Unresolved
- DefaultInput.ini overrides some engine defaults (expected for prototype input binding).
- Build camera does not rotate around mouse cursor (pan/rotate only); fine for v0.1.
- No camera animation beyond SetViewTargetWithBlend (0.3s); acceptable prototype.
- Movement input is partially delegated to character's own SetupPlayerInputComponent bindings; controller gates mode only.

## Compromises
- No polished HUD, no blueprint camera assets, no animation blending.
- Build camera pan uses fixed speed (not mouse-drag); acceptable per spec.
- No multi-touch / gamepad camera controls.

## Manual Setup Required
- Ensure Config/DefaultInput.ini is included in project (already edited).
- Confirm /Engine/BasicShapes/Cube available (existing).
- No external assets needed.

## Top 5 Manual Review Points
1. Character spring arm bUsePawnControlRotation must stay true for mouse look; verify after build.
2. Build camera spawned only when Build active; verify no duplicate actors after rapid B presses.
3. Mouse cursor visible in Build (bShowMouseCursor = true) for placement; hidden in Live.
4. Wall preview still appears only when Previewing state; destroyed when exiting build.
5. Sprint disables automatically when entering Build (IsBuildModeActive check); verify no residual sprint speed.

## Recommended Next Step
Add mouse-drag rotation for build camera (MMB drag) and optional Q/E fine rotation; extend to support property origin offset for larger properties.

## Stopping Condition Verification
1. Compile success ✓
2. Existing tests pass ✓
3. New deterministic camera/math tests pass ✓
4. WASD live movement implemented ✓
5. Mouse look implemented ✓
6. Sprint implemented ✓
7. Third-person live camera exists ✓
8. Build camera usable ✓
9. Build camera pan ✓
10. Build camera zoom ✓
11. Build camera rotation ✓
12. Live↔Build transition implemented ✓
13. Normal movement disabled in build ✓
14. Wall placement controls preserved ✓
15. No per-tick camera spawning ✓
16. No commit or push ✓
