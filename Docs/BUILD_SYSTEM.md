# Proxima Build System

## Coordinate and unit convention
- Unreal world unit: 1 unit = 1 centimeter.
- Proxima architectural data is stored in centimeters.
- Property-local wall endpoints use XY as the horizontal plane.
- Z is reserved for elevation/storey height.
- Never multiply centimeters by `0.01` when converting persistent geometry to Unreal world coordinates.

## Measurement input
`UProximaMeasurement` supports meters and centimeters. Bare numeric input is currently treated as meters. Examples:
- `3.65` -> 365 cm
- `3.65 m` -> 365 cm
- `365 cm` -> 365 cm
- `3,65m` -> 365 cm

## Snapping
Supported configuration levels are 1, 5, 10, 25, 50, and 100 cm, plus disabled snapping. Grid snapping is stateless; the measurement subsystem owns the selected resolution.

## Walls
`FProximaWallData` contains persistent geometry, stable IDs, floor/building ownership, height, thickness, side materials, openings, and topology references. Runtime wall Actors are not stored in save data.

Zero-length walls are rejected by the active building model.

## Commands
Build mutations use reversible `UProximaBuildCommand` objects. `UProximaCommandManager` injects the active building model before execution and only records commands that execute successfully.

Initial wall commands:
- create wall
- delete wall
- modify wall

## Next build-system layer
The next implementation should be a dedicated wall-placement session/tool that owns:
- placement start point
- cursor/world trace
- snapped preview endpoint
- optional exact-length override
- preview visualization
- confirm/cancel

Confirming placement should create a wall command. Preview Actors must never become the persistent source of truth.
