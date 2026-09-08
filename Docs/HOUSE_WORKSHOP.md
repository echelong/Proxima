# Proxima house-building workshop

This is a source milestone built from the uploaded local project, not a finished
life simulation or a packaged Linux game. The UE 5.8.2 Linux editor build passed;
asset bootstrap, Unreal automation, visual inspection, and performance testing
remain required.

The compiler naming fixes passed the user's second build. Starter-material
generation then failed on the Noise node's `Position` connection. This revision
uses the material API's first-input connection and improves failure diagnostics.
Run the workshop script again after applying `Proxima-bootstrap-fix.patch` to the
previous source, or extract the current full archive. See
`AUTONOMOUS_LAST_VERIFY.md` for the supplied logs and outstanding checks.

## First Linux test

Extract the returned source archive into a separate directory. Keep the original
`~/Projects/Proxima` checkout and its local commits intact until the test is green.
From the extracted `Proxima` directory run:

```bash
bash Tools/linux_workshop.sh --play
```

The runner uses `~/Unreal/UE_5.8.2` by default. To use another installed engine:

```bash
PROXIMA_UE_ROOT="$HOME/Unreal/UE_5.8.2" bash Tools/linux_workshop.sh --play
```

It builds the editor module, generates starter materials and the Workshop map,
runs all authored Proxima Unreal tests, and opens a game window only after a
complete passing report. Each run gets a new `Saved/WorkshopChecks/run.*` folder.
A failed build, bootstrap, missing report, missing test, or failed test stops it.
Send that run's logs when something fails. No Git operations, engine edits,
package installation, or third-party downloads occur in this runner.

Other modes:

- `--preflight`: check configuration and engine paths only.
- `--verify`: editor build, asset generation and Unreal tests, without opening the game.
- `--package`: also cook and package a Development Linux executable under `Dist/Workshop`.

The first startup may need shader compilation. Python runs only in the editor
during asset creation. Gameplay, construction rules, input and persistence are C++.
The setup uses Epic's [editor Python workflow](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python),
[material editing API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary?application_version=5.6),
and [level editor subsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/LevelEditorSubsystem?application_version=5.6).

## What to try first

1. Press **B** to open construction mode. The player stops moving while you build.
2. Click **Add example home to empty lot**. This adds an 8 × 6 m interior, a
   partition, two doorways, four windows, a floor and a flat roof. It is one undo step.
3. Press **B** to walk toward the entrance. Move with WASD; Shift sprints.
   The default eye-level camera is 1.6 m above the standing surface. V toggles orbit view.
4. Return to Build. Roofs are hidden for editing; enable **Show roofs while building** to inspect them.
5. Select a wall, change its colour, add another window, then undo and redo.
6. Save with F5, modify the layout, and load with F9.

The example requires an empty current model and never overwrites an existing home.

## Construction tools

| Key | Tool | Interaction |
| --- | --- | --- |
| 1 | Select | Click a wall or horizontal surface. Delete removes it. |
| 2 | Wall | Click start and end. Further walls chain from the last endpoint. |
| 3 | Room | Click one internal corner, then the opposite corner. Adds four walls, a floor and a flat roof. |
| 4 / O | Doorway | Point at the wall where the opening should go and click. |
| 5 | Window | Point at the wall and click. Width, height and sill are editable. |
| 6 | Floor | Place a rectangular 20 cm slab with its top at floor level. |
| 7 | Flat roof | Place a rectangular 20 cm slab with its underside at the chosen wall height. |

Right-click or Escape cancels a gesture. Ctrl+Z / Ctrl+Y undo and redo.
WASD pans the build camera, middle mouse rotates it, and the wheel zooms.
UI clicks are separated from world placement. Colour changes affect the selected
wall in Select mode or become the finish for new walls when nothing is selected.

Measurement fields accept `3.65`, `3.65 m`, `365 cm`, and comma decimals.
Bare numbers are metres. Room width/depth describe **clear internal space**;
wall thickness is placed outside that rectangle. Individual wall lengths are
measured along their centre lines. Floor/roof tool dimensions are slab extents.
Set exact wall length to 0 to follow the cursor. Set room width/depth to 0 to draw
that dimension freely. Grid choices are off, 1, 5, 10, 25, 50 and 100 cm. Existing
endpoints have snap priority unless an exact length or angle constraint overrides them.

## Implementation and persistence

- `UProximaBuildingManager` owns authoritative walls and rectangular slabs.
  Every create, edit and load validates finite dimensions, IDs, opening bounds,
  opening overlap, duplicate walls, and overlapping surfaces before mutation.
- `UProximaWorkshopComponent` owns construction gestures and transient representations.
  `AProximaPlayerController` remains the single input owner and routes Live/Build input.
- `ProximaGeometryKernel.h` is production C++ used by the wall renderer and room
  builder. Its independent Linux tests execute the same geometry functions.
- Walls use instanced solid boxes around all openings. Window sills stay solid;
  doorways have no invisible full-wall collider. Right-angle endpoint joins receive
  an exterior corner fill. Non-right-angle mitres and wall-intersection unions are future work.
- A bounded preview pool is reused. Selection changes tint without recreating walls.
  Model edits still rebuild representations, so large-city performance is unproven.
- A room gesture changes walls and slabs atomically in one command and one notification.
  Undo/redo preserves IDs. F9 establishes a new command history.
- Save format V2 adds `Slabs`. V1 wall-only saves are accepted with an empty slab
  array. Existing invalid/overlapping opening data is rejected rather than silently
  rewritten. Save slots live in this project's `Saved/SaveGames` directory.

If an old save contains openings that the previous renderer ignored, loading may
now reject it. Keep that save for diagnosis. Do not delete it or repeatedly
resave over it. The uploaded archive did not include existing `Saved` data.

## Verification record for this source

Completed in the development workspace:

- C++17 compile with `-Wall -Wextra -Werror -pedantic`.
- AddressSanitizer and UndefinedBehaviorSanitizer: 230,107 assertions across
  deterministic cases and 1,000 random layouts. Tests verify area conservation,
  no solid overlap with openings, stable output order, window sills, bounds,
  invalid dimensions, internal room dimensions and corner fills.
- Leak checking was disabled because this container disallows the process
  inspection LeakSanitizer requires. No leak-check pass is claimed.
- Six report-validator tests reject incomplete, failed, corrupt and duplicate reports.
- Input configuration, Python syntax, shell syntax and whitespace review passed.

**Not run here:** UnrealHeaderTool, UE C++ compilation/linking, the 15 Unreal
Proxima tests, starter-asset generation inside Unreal, native rendering, collision
playtesting, Linux cooking/packaging, or FPS/VRAM measurements. No binary release
or completed game is claimed. Earlier worklog passes apply to earlier source only.

## Required manual gate after a green build

- All four Live movement directions work, including after editing text fields.
- Enter Build while walking: the player stops. Repeated mode switches create one build camera.
- Pointer movement alone does not rotate the build camera; MMB does.
- Clicking/typing in the panel never places a wall or triggers a numbered tool.
- A 6 × 4 m room with 15 cm walls has those internal clear dimensions.
- All right-angle corners are closed. Inspect at eye level as well as overhead.
- Two or more openings render in the same wall; windows retain their sill.
- The capsule can walk through a 90 cm doorway. It cannot walk through a window or a solid wall.
- Undo/redo a room, opening, deletion and colour change. Confirm geometry and collision update together.
- F5 -> modify -> F9 restores the whole home, including roofs and GUID-backed data.
- Roof visibility changes only the construction view; roofs return when walking.
- Check sun, exposure, shadows, translucency and text contrast at 1600 × 900.
- Record framerate and memory on the Ryzen 7 3700X / RTX 2070 SUPER before changing quality targets.

## Remaining game scope

This milestone supplies single-storey construction tools and an inspection loop.
It does not yet supply a photorealistic asset catalogue, furnishing, animated
residents, terrain editing, curved walls, robust room topology/merging, stairs,
multi-storey editing, pitched roofs, doors with operable leaves, vehicles, AI
reference-image creation, neighbourhood visits, or multiplayer. Those remain
Proxima goals. Establish the compiled and visually tested builder before extending them.

The existing handoff's Git gate remains: compile, run all Proxima automation,
and pass the manual playtest before pushing these source changes.
