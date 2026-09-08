# Current handoff notice

The latest source milestone is documented in [HOUSE_WORKSHOP.md](HOUSE_WORKSHOP.md).
Read [AUTONOMOUS_LAST_VERIFY.md](AUTONOMOUS_LAST_VERIFY.md) before continuing.
The historical handoff below describes the preceding wall-only implementation.
Its compile/playtest-before-push requirement remains in force.

---

# Proxima — Live/Build First-Playable Handoff

## Current review state

This file reflects the independent manual code review performed after local checkpoint `63f4345`.
The review patch must still be compiled and PIE-tested on the Fedora/UE 5.8.2 workstation before merge/push.

## Architecture

- Persistent `FProximaWallData` in `UProximaBuildingManager` remains authoritative.
- Runtime wall Actors are rebuilt representations and are never save-file truth.
- Stable wall IDs remain GUID-backed.
- Undo/redo remains command-based.
- Build Mode is an interaction mode in `UProximaInteractionSubsystem`, not a GameMode swap.
- Unreal world coordinates are centimetres: 1 UU = 1 cm.
- The engine BasicShapes cube is 100 cm; `LengthCm / 100` is mesh scaling only.

## Input ownership

The controller is the single gameplay-input owner.

Legacy input is used deliberately:

- `DefaultPlayerInputClass=/Script/Engine.PlayerInput`
- `DefaultInputComponentClass=/Script/Engine.InputComponent`
- the unused Enhanced Input project dependency/plugin is removed

`AProximaPlayerController` binds and routes:

- `MoveForward`: W/S
- `MoveRight`: D/A
- `Turn`: MouseX
- `LookUp`: MouseY
- `BuildZoom`: MouseWheelAxis
- Shift press/release: sprint
- B: Live/Build toggle
- MMB press/release: Build camera rotation gate
- LMB: start/confirm wall
- RMB/Escape: cancel current wall chain/preview
- Ctrl+Z / Ctrl+Y: undo/redo
- F5/F9: prototype save/load

`AProximaCharacter::SetupPlayerInputComponent` intentionally owns no gameplay bindings.
In Live mode the controller forwards movement axes to the Character. In Build mode the same axes pan the Build camera, so the pawn cannot move behind the construction view.

## Live camera

- Character owns spring arm + camera.
- Camera follows controller yaw/pitch.
- Character movement vectors use control yaw and the Character rotates toward movement direction.
- Walk and sprint speeds remain configurable.
- Entering Build clears sprint state.

## Build camera

- Controller owns one transient `AProximaBuildCamera` Actor.
- It is spawned once on first Build entry and preserved across toggles.
- WASD pans relative to Build-camera yaw.
- Mouse wheel changes spring-arm distance without frame-time scaling.
- MMB + MouseX rotates yaw; ordinary mouse movement alone does not rotate the Build camera.
- Build view uses a fixed downward pitch and bounded zoom.

## Wall placement

- Cursor projection now intersects the deprojected mouse ray directly with the horizontal build plane. Existing wall meshes cannot distort the cursor XY by intercepting a visibility trace first.
- Endpoint snapping still takes precedence over grid snapping.
- Confirming a wall chains naturally: the confirmed snapped endpoint becomes the next wall start.
- RMB/Escape breaks the chain and returns to choosing a fresh start point.
- `PreviewLengthM` is reset correctly between placement states.
- A lightweight world-space debug label displays the current preview length such as `3.00 m`.
- Duplicate wall geometry is preview-rejected and, importantly, is also rejected authoritatively by `UProximaBuildingManager`, so other callers cannot bypass the rule.

## Save/load

- Save payload remains versioned V1 and contains persistent data only.
- F5 saves the current persistent wall model.
- F9 loads through an atomic `ReplaceWalls` operation, causing one persistent-state broadcast/runtime rebuild rather than reset + N incremental rebuilds + an extra rebuild.
- Loading clears command history because undo/redo commands from the pre-load timeline are no longer valid.
- A Build-mode load resets the active wall preview/chain.
- Save automation uses a unique slot and deletes it after the test.

## Automated coverage added/strengthened by manual review

Existing test names remain stable. Assertions now additionally cover:

- direct horizontal build-plane ray intersection
- chained wall-session state
- direction-independent duplicate-wall geometry
- atomic loaded-wall replacement
- duplicate replacement rejection without partial mutation
- automation save-slot cleanup

## Manual PIE gate still required

Before merge/push, verify in PIE:

1. Live W, A, S, D all move in the expected directions.
2. Mouse yaw/pitch works in Live.
3. Shift sprints and never remains stuck after Build toggles.
4. B repeatedly toggles Live/Build without camera duplication or cursor corruption.
5. Build W/A/S/D pan relative to the current Build-camera yaw.
6. Mouse alone moves the wall cursor but does not rotate the Build camera.
7. Hold MMB and move the mouse: Build camera rotates.
8. Mouse wheel zooms in/out and respects bounds.
9. LMB start/confirm works and subsequent walls automatically chain from the last endpoint.
10. RMB/Escape breaks a chain and allows choosing a fresh start.
11. The preview length label is visible and numerically plausible.
12. Wall cursor remains on the construction plane even when hovering over an existing wall.
13. Duplicate walls cannot be confirmed/created.
14. Ctrl+Z / Ctrl+Y preserve sensible placement state.
15. F5 save -> modify layout -> F9 load restores the saved layout.
16. After F9, old pre-load undo history cannot mutate the restored timeline.

## Git policy

Do not push this manual-review patch until UE 5.8.2 compilation, the full `Proxima` automation suite, and the manual PIE gate above are green.
