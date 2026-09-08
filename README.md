# Proxima

Proxima is a 3D open-world architectural, interior-design and life simulation game.

## Vision

Build real-scale homes with precise measurements, furnish and landscape them,
walk freely through an open neighborhood, visit other homes, drive vehicles,
and share creations with other players.

## Core goals

- Accurate real-world building dimensions
- Powerful but accessible Build Mode
- Seamless 3D open world
- Interior and exterior customization
- Landscaping
- User-created and downloadable content
- Vehicles
- Neighborhood exploration
- Community house and asset sharing
- Future AI-assisted asset creation

## Engine

Unreal Engine 5

Core gameplay systems will primarily use C++, with Blueprints used where
rapid visual iteration is useful.

## Current development milestone

The house-building workshop passed UE 5.8.2 Linux editor compilation and linking.
This revision fixes the subsequent starter-material bootstrap error; asset
generation, Unreal automation and native playtesting still need to pass. It adds metric
construction controls, rectangular rooms/floors/flat roofs, multiple wall openings,
undo/redo, save/load, and an example home.

See [the workshop guide](Docs/HOUSE_WORKSHOP.md) and
[current verification state](Docs/AUTONOMOUS_LAST_VERIFY.md).

```bash
bash Tools/linux_workshop.sh --play
```
