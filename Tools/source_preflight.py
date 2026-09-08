#!/usr/bin/env python3
"""Small configuration checks that catch the input/plugin mismatch seen in Proxima."""
import ast
import configparser
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    project = json.loads((ROOT / 'Proxima.uproject').read_text())
    if project.get('EngineAssociation') != '5.8':
        raise ValueError('This source targets Unreal Engine 5.8; inspect the target settings before changing versions.')
    enhanced = [p for p in project.get('Plugins', []) if p.get('Name') == 'EnhancedInput']
    if len(enhanced) != 1 or enhanced[0].get('Enabled') is not False:
        raise ValueError('Proxima.uproject must explicitly disable EnhancedInput to prevent editor input-class upgrades. '
                         'Close Unreal and run: python3 Tools/repair_legacy_input.py')
    engine_config = configparser.ConfigParser(strict=False, interpolation=None)
    engine_config.read(ROOT / 'Config/DefaultEngine.ini')
    if engine_config.get('SystemSettings', 'EnhancedInput.bEnableAutoUpgrade', fallback=None) != '0':
        raise ValueError('DefaultEngine.ini must set EnhancedInput.bEnableAutoUpgrade=0 under [SystemSettings]. '
                         'Close Unreal and run: python3 Tools/repair_legacy_input.py')
    input_config = (ROOT / 'Config/DefaultInput.ini').read_text()
    required = {
        'DefaultPlayerInputClass': '/Script/Engine.PlayerInput',
        'DefaultInputComponentClass': '/Script/Engine.InputComponent',
    }
    for key, value in required.items():
        actual = re.findall(r'^' + re.escape(key) + r'=(.*)$', input_config, re.MULTILINE)
        if actual != [value]:
            raise ValueError(f'{key} must occur once and use {value}; found {actual}. '
                             'Close Unreal and run: python3 Tools/repair_legacy_input.py')
    for axis, key, scale in [('MoveForward', 'W', 1), ('MoveForward', 'S', -1),
                             ('MoveRight', 'D', 1), ('MoveRight', 'A', -1)]:
        pattern = rf'AxisName="{axis}",Scale=(-?[\d.]+),Key={key}\)'
        matches = re.findall(pattern, input_config)
        if len(matches) != 1 or float(matches[0]) != scale:
            raise ValueError(f'Wrong or duplicate {key} movement mapping')
    for path in (ROOT / 'Tools').glob('*.py'):
        ast.parse(path.read_text(), filename=str(path))
    if not (ROOT / 'Source/Proxima/Public/Building/ProximaGeometryKernel.h').is_file():
        raise ValueError('Production geometry kernel is missing')
    print('PASS: Unreal target, EnhancedInput disabled, automatic upgrade off, input ownership, WASD and Python syntax.')
    print('This check does not compile Unreal C++ or verify rendering.')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, SyntaxError, configparser.Error) as error:
        print('PREFLIGHT FAILED: ' + str(error), file=sys.stderr)
        sys.exit(1)
