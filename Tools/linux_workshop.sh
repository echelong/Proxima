#!/usr/bin/env bash
# Build, create starter assets, verify all Unreal tests, then optionally play/package.
# Does not modify branches, push to GitHub, install packages, or alter the engine.
set -euo pipefail
proxima_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
proxima_ue_root="${PROXIMA_UE_ROOT:-$HOME/Unreal/UE_5.8.2}"
proxima_project="$proxima_root/Proxima.uproject"
proxima_mode="${1:---verify}"
case "$proxima_mode" in
    --verify|--play|--package|--preflight) ;;
    *) printf 'Usage: bash Tools/linux_workshop.sh [--verify|--play|--package|--preflight]\n' >&2; exit 2 ;;
esac
if [[ $# -gt 1 ]]; then printf 'Only one mode argument is supported.\n' >&2; exit 2; fi
command -v python3 >/dev/null || { printf 'Python 3 is required to read the test report.\n' >&2; exit 2; }
python3 "$proxima_root/Tools/source_preflight.py"
proxima_build="$proxima_ue_root/Engine/Build/BatchFiles/Linux/Build.sh"
proxima_editor="$proxima_ue_root/Engine/Binaries/Linux/UnrealEditor"
proxima_editor_cmd="$proxima_ue_root/Engine/Binaries/Linux/UnrealEditor-Cmd"
if [[ ! -x "$proxima_editor_cmd" ]]; then proxima_editor_cmd="$proxima_editor"; fi
if [[ ! -f "$proxima_build" || ! -x "$proxima_editor" ]]; then
    printf 'Unreal Engine is not available at: %s\nSet PROXIMA_UE_ROOT to your Unreal installation directory.\n' "$proxima_ue_root" >&2
    exit 3
fi
if [[ "$proxima_mode" == --preflight ]]; then
    printf 'Engine found: %s\nNo build or game test has run.\n' "$proxima_ue_root"
    exit 0
fi
proxima_jobs="${PROXIMA_BUILD_JOBS:-4}"
if [[ ! "$proxima_jobs" =~ ^[1-9][0-9]*$ ]]; then printf 'PROXIMA_BUILD_JOBS must be a positive integer.\n' >&2; exit 2; fi
mkdir -p "$proxima_root/Saved/WorkshopChecks"
proxima_log_dir="$(mktemp -d "$proxima_root/Saved/WorkshopChecks/run.XXXXXX")"
# Unique run folders ensure an earlier green report can never verify this build.
printf 'Build and test logs: %s\n' "$proxima_log_dir"
trap 'proxima_exit=$?; if (( proxima_exit != 0 )); then printf "Stopped with exit %s. Logs: %s\n" "$proxima_exit" "$proxima_log_dir" >&2; fi' EXIT
cp "$proxima_project" "$proxima_log_dir/project-before.uproject"
cp "$proxima_root/Config/DefaultInput.ini" "$proxima_log_dir/input-before.ini"
cp "$proxima_root/Config/DefaultEngine.ini" "$proxima_log_dir/engine-before.ini"
bash "$proxima_build" ProximaEditor Linux Development "$proxima_project" -WaitMutex -NoHotReloadFromIDE "-MaxParallelActions=$proxima_jobs" \
    2>&1 | tee "$proxima_log_dir/build.log"
"$proxima_editor_cmd" "$proxima_project" /Engine/Maps/Entry \
    "-ExecutePythonScript=$proxima_root/Tools/bootstrap_workshop.py" \
    -unattended -nop4 -nosplash -NullRHI -stdout -FullStdOutLogOutput \
    "-abslog=$proxima_log_dir/bootstrap-engine.log" \
    2>&1 | tee "$proxima_log_dir/bootstrap.log"
python3 - "$proxima_log_dir/bootstrap.log" <<'PY'
from pathlib import Path
import sys
log = Path(sys.argv[1]).read_text(errors='replace')
if 'PROXIMA_WORKSHOP_BOOTSTRAP_OK' not in log:
    raise SystemExit('Starter scene generation did not complete. See bootstrap.log.')
PY
python3 "$proxima_root/Tools/source_preflight.py" 2>&1 | tee "$proxima_log_dir/preflight-after-bootstrap.log"
"$proxima_editor_cmd" "$proxima_project" /Game/Proxima/Maps/Workshop \
    -unattended -nop4 -nosplash -NullRHI -stdout -FullStdOutLogOutput \
    '-ExecCmds=Automation RunTests Proxima' '-TestExit=Automation Test Queue Empty' \
    "-ReportExportPath=$proxima_log_dir/Automation" "-abslog=$proxima_log_dir/tests-engine.log" \
    2>&1 | tee "$proxima_log_dir/tests.log"
python3 "$proxima_root/Tools/check_automation_report.py" "$proxima_log_dir/Automation/index.json" "$proxima_root/Source"
python3 "$proxima_root/Tools/source_preflight.py" 2>&1 | tee "$proxima_log_dir/preflight-after-tests.log"
printf 'Unreal editor build and automation passed. Manual graphics/input testing is still required.\n'
if [[ "$proxima_mode" == --package ]]; then
    bash "$proxima_ue_root/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
        "-project=$proxima_project" -nop4 -platform=Linux -clientconfig=Development \
        -build -cook -stage -pak -archive -utf8output \
        '-map=/Game/Proxima/Maps/Workshop' "-archivedirectory=$proxima_root/Dist/Workshop" \
        2>&1 | tee "$proxima_log_dir/package.log"
    printf 'Packaged Linux output: %s\n' "$proxima_root/Dist/Workshop"
elif [[ "$proxima_mode" == --play ]]; then
    "$proxima_editor" "$proxima_project" /Game/Proxima/Maps/Workshop \
        -game -windowed -ResX=1600 -ResY=900 -log \
        "-abslog=$proxima_log_dir/play.log"
fi
