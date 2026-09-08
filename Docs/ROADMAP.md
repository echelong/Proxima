# Current milestone status

House workshop source implementation: complete for the scope in HOUSE_WORKSHOP.md.
Unreal build, native automation, graphics, collision, packaging and performance:
not verified for this source. Passing these gates is the next priority.

The longer roadmap below is historical planning. It is not evidence of completed
or tested game features. Rooms, rectangular slabs, a build panel and multiple
openings are now authored; robust topology and full multi-storey building are still future work.

---

# Proxima Roadmap

## V0.1 - Foundation stabilization (current)
- [x] Unreal project/module scaffold
- [x] GameMode, GameState, PlayerController and Character classes
- [x] Live/Build interaction-mode state
- [x] Metric conversion and configurable snapping foundation
- [x] Property/building/floor/wall/opening persistent models
- [x] GUID-backed persistent identifiers
- [x] Command/undo architecture for wall mutations
- [x] Versioned `USaveGame` payload, first real format V1
- [x] Built-in catalog data-asset foundation
- [x] Measurement/snapping/wall-data automation tests authored
- [ ] Compile with Unreal Engine 5.5 and resolve actual compiler/UHT diagnostics
- [ ] Add a real GameInstance-backed undo/redo automation test

## V0.2 - First playable wall slice
- [ ] Enhanced Input assets/mapping contexts
- [ ] Build camera/pawn strategy
- [ ] Wall placement session: start/preview/snap/exact length/confirm/cancel
- [ ] Runtime wall visualization reconstructed from `FProximaWallData`
- [ ] Basic material assignment
- [ ] Save active property and reconstruct it on load
- [ ] Switch to Live Mode and walk through the result

## V0.3 - Rooms and floors
- [ ] Robust wall endpoint/topology maintenance
- [ ] Closed-loop room detection
- [ ] Floor surfaces
- [ ] Multi-storey editing
- [ ] Stairs data model and placement

## V0.4 - Openings
- [ ] Door/window placement
- [ ] Opening validation along wall segments
- [ ] Runtime wall geometry with openings

## V0.5 - Catalog and materials
- [ ] Material primary assets/data assets
- [ ] Catalog browser UI
- [ ] Material preview
- [ ] Runtime/UGC registry layer

## V0.6 - Neighborhood/world foundation
- [ ] Multiple lots
- [ ] World Partition/streaming plan
- [ ] Day/night foundation
- [ ] NPC simulation architecture

## Later
- Landscaping and terrain tools
- Vehicles and bicycles
- Weather and seasons
- Neighborhood visits/sharing
- User-created asset import pipeline
- Reference-image-assisted asset creation with rights/safety checks
- Multiplayer after the single-player building model is stable
