# Proxima Game Design

## Vision
True-scale architecture + easy build mode + open-world life simulation.

## Core Loop
Player loads property → enters Build Mode → draws exact-dimension walls → saves → switches to Live Mode → walks through creation → explores neighborhood.

## Key Design Principles
- Metric measurements (cm internal, m user-facing)
- Persistent building data separated from runtime Actor pointers
- Modular subsystem architecture (measurement, build, catalog, save, interaction)
- Command architecture for build actions (execute / undo / redo)
- Versioned save format from first version (V2 current)
- No single giant manager; subsystems handle domains
