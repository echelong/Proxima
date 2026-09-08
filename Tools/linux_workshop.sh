#!/usr/bin/env bash
# Build, create starter assets, verify all Unreal tests, then optionally play/package.
# Uses explicit exit-code checks. No automatic shell exit-on-error behavior.

proxima_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
proxima_ue_root="${PROXIMA_UE_ROOT:-$HOME/Unreal/UE_5.8.2}"
proxima_project="$proxima_root/Proxima.uproject"
proxima_mode="${1:---verify}"

case "$proxima_mode" in
    --verify|--play|--package|--preflight)
        ;;
    *)
        printf 'Usage: bash Tools/linux_workshop.sh [--verify|--play|--package|--preflight]\n' >&2
        exit 2
        ;;
esac

if [ "$#" -gt 1 ]; then
    printf 'Only one mode argument is supported.\n' >&2
    exit 2
fi

if ! command -v python3 >/dev/null 2>&1; then
    printf 'Python 3 is required to read the test report.\n' >&2
    exit 2
fi

python3 "$proxima_root/Tools/source_preflight.py"
preflight_status=$?

if [ "$preflight_status" -ne 0 ]; then
    printf 'Initial source preflight failed with exit %s.\n' "$preflight_status" >&2
    exit "$preflight_status"
fi

proxima_build="$proxima_ue_root/Engine/Build/BatchFiles/Linux/Build.sh"
proxima_editor="$proxima_ue_root/Engine/Binaries/Linux/UnrealEditor"
proxima_editor_cmd="$proxima_ue_root/Engine/Binaries/Linux/UnrealEditor-Cmd"

if [ ! -x "$proxima_editor_cmd" ]; then
    proxima_editor_cmd="$proxima_editor"
fi

if [ ! -f "$proxima_build" ] || [ ! -x "$proxima_editor" ]; then
    printf 'Unreal Engine is not available at: %s\n' "$proxima_ue_root" >&2
    printf 'Set PROXIMA_UE_ROOT to your Unreal installation directory.\n' >&2
    exit 3
fi

if [ "$proxima_mode" = "--preflight" ]; then
    printf 'Engine found: %s\n' "$proxima_ue_root"
    printf 'No build or game test has run.\n'
    exit 0
fi

proxima_jobs="${PROXIMA_BUILD_JOBS:-4}"

if ! [[ "$proxima_jobs" =~ ^[1-9][0-9]*$ ]]; then
    printf 'PROXIMA_BUILD_JOBS must be a positive integer.\n' >&2
    exit 2
fi

mkdir -p "$proxima_root/Saved/WorkshopChecks"
mkdir_status=$?

if [ "$mkdir_status" -ne 0 ]; then
    printf 'Could not create WorkshopChecks directory.\n' >&2
    exit "$mkdir_status"
fi

proxima_log_dir="$(mktemp -d "$proxima_root/Saved/WorkshopChecks/run.XXXXXX")"
mktemp_status=$?

if [ "$mktemp_status" -ne 0 ] || [ ! -d "$proxima_log_dir" ]; then
    printf 'Could not create verification run directory.\n' >&2
    exit 4
fi

printf 'Build and test logs: %s\n' "$proxima_log_dir"

trap '
proxima_exit=$?
if [ "$proxima_exit" -ne 0 ]; then
    printf "Stopped with exit %s. Logs: %s\n" "$proxima_exit" "'"$proxima_log_dir"'" >&2
fi
' EXIT

if ! cp "$proxima_project" "$proxima_log_dir/project-before.uproject"; then
    exit 4
fi

if ! cp "$proxima_root/Config/DefaultInput.ini" "$proxima_log_dir/input-before.ini"; then
    exit 4
fi

if ! cp "$proxima_root/Config/DefaultEngine.ini" "$proxima_log_dir/engine-before.ini"; then
    exit 4
fi

echo
echo "=== BUILD ==="

bash "$proxima_build" \
    ProximaEditor \
    Linux \
    Development \
    "$proxima_project" \
    -WaitMutex \
    -NoHotReloadFromIDE \
    "-MaxParallelActions=$proxima_jobs" \
    2>&1 | tee "$proxima_log_dir/build.log"

build_status=${PIPESTATUS[0]}

if [ "$build_status" -ne 0 ]; then
    printf 'Unreal build failed with exit %s.\n' "$build_status" >&2
    exit "$build_status"
fi

echo
echo "=== BOOTSTRAP WORKSHOP ASSETS ==="

"$proxima_editor_cmd" \
    "$proxima_project" \
    /Engine/Maps/Entry \
    "-ExecutePythonScript=$proxima_root/Tools/bootstrap_workshop.py" \
    -unattended \
    -nop4 \
    -nosplash \
    -NullRHI \
    -stdout \
    -FullStdOutLogOutput \
    "-abslog=$proxima_log_dir/bootstrap-engine.log" \
    2>&1 | tee "$proxima_log_dir/bootstrap.log"

bootstrap_engine_status=${PIPESTATUS[0]}

python3 - "$proxima_log_dir/bootstrap.log" <<'PY'
from pathlib import Path
import sys

log = Path(sys.argv[1]).read_text(errors="replace")

if "PROXIMA_WORKSHOP_BOOTSTRAP_OK" not in log:
    print("Starter scene generation did not complete. See bootstrap.log.", file=sys.stderr)
    raise SystemExit(1)

print("PASS: Workshop bootstrap completion marker found.")
PY

bootstrap_marker_status=$?

if [ "$bootstrap_marker_status" -ne 0 ]; then
    exit "$bootstrap_marker_status"
fi

if [ "$bootstrap_engine_status" -ne 0 ]; then
    printf 'NOTE: Unreal bootstrap process returned %s, but the required completion marker is present.\n' \
        "$bootstrap_engine_status"
fi

echo
echo "=== PREFLIGHT AFTER BOOTSTRAP ==="

python3 "$proxima_root/Tools/source_preflight.py" \
    2>&1 | tee "$proxima_log_dir/preflight-after-bootstrap.log"

post_bootstrap_status=${PIPESTATUS[0]}

if [ "$post_bootstrap_status" -ne 0 ]; then
    printf 'Post-bootstrap preflight failed with exit %s.\n' "$post_bootstrap_status" >&2
    exit "$post_bootstrap_status"
fi

echo
echo "=== UNREAL AUTOMATION ==="

"$proxima_editor_cmd" \
    "$proxima_project" \
    /Game/Proxima/Maps/Workshop \
    -unattended \
    -nop4 \
    -nosplash \
    -NullRHI \
    -stdout \
    -FullStdOutLogOutput \
    '-ExecCmds=Automation RunTests Proxima' \
    '-TestExit=Automation Test Queue Empty' \
    "-ReportExportPath=$proxima_log_dir/Automation" \
    "-abslog=$proxima_log_dir/tests-engine.log" \
    2>&1 | tee "$proxima_log_dir/tests.log"

automation_engine_status=${PIPESTATUS[0]}

echo
echo "=== VALIDATE AUTOMATION REPORT ==="

python3 \
    "$proxima_root/Tools/check_automation_report.py" \
    "$proxima_log_dir/Automation/index.json" \
    "$proxima_root/Source"

automation_report_status=$?

if [ "$automation_report_status" -ne 0 ]; then
    printf 'Automation report validation failed with exit %s.\n' \
        "$automation_report_status" >&2
    exit "$automation_report_status"
fi

if [ "$automation_engine_status" -ne 0 ]; then
    printf 'NOTE: Unreal automation process returned %s after producing a fully successful report.\n' \
        "$automation_engine_status"
fi

echo
echo "=== FINAL PREFLIGHT ==="

python3 "$proxima_root/Tools/source_preflight.py" \
    2>&1 | tee "$proxima_log_dir/preflight-after-tests.log"

final_preflight_status=${PIPESTATUS[0]}

if [ "$final_preflight_status" -ne 0 ]; then
    printf 'Final source preflight failed with exit %s.\n' \
        "$final_preflight_status" >&2
    exit "$final_preflight_status"
fi

printf '\nPASS: Unreal editor build, bootstrap and automation verified.\n'
printf 'Manual graphics/input testing is still required.\n'

if [ "$proxima_mode" = "--package" ]; then

    echo
    echo "=== PACKAGE LINUX BUILD ==="

    bash "$proxima_ue_root/Engine/Build/BatchFiles/RunUAT.sh" \
        BuildCookRun \
        "-project=$proxima_project" \
        -nop4 \
        -platform=Linux \
        -clientconfig=Development \
        -build \
        -cook \
        -stage \
        -pak \
        -archive \
        -utf8output \
        '-map=/Game/Proxima/Maps/Workshop' \
        "-archivedirectory=$proxima_root/Dist/Workshop" \
        2>&1 | tee "$proxima_log_dir/package.log"

    package_status=${PIPESTATUS[0]}

    if [ "$package_status" -ne 0 ]; then
        printf 'Linux packaging failed with exit %s.\n' "$package_status" >&2
        exit "$package_status"
    fi

    printf 'Packaged Linux output: %s\n' "$proxima_root/Dist/Workshop"

elif [ "$proxima_mode" = "--play" ]; then

    echo
    echo "=== LAUNCH PROXIMA ==="

    env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}" \
        "$proxima_editor" \
        "$proxima_project" \
        /Game/Proxima/Maps/Workshop \
        -game \
        -windowed \
        -ResX=1600 \
        -ResY=900 \
        -log \
        "-abslog=$proxima_log_dir/play.log"

    play_status=$?

    if [ "$play_status" -ne 0 ]; then
        printf 'Proxima play process exited with %s.\n' "$play_status" >&2
        exit "$play_status"
    fi
fi

exit 0
