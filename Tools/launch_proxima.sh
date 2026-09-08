#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
UE_ROOT="${PROXIMA_UE_ROOT:-$HOME/Unreal/UE_5.8.2}"

export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"

exec "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" \
    "$ROOT/Proxima.uproject" \
    "/Game/Proxima/Maps/Workshop"
