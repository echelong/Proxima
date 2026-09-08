#!/usr/bin/env python3
"""Restore Proxima's legacy input configuration. Close Unreal before running.

Usage: python3 repair_legacy_input.py [path/to/Proxima]
Only Proxima.uproject, Config/DefaultInput.ini and Config/DefaultEngine.ini are
edited. Changed originals are backed up under Saved/WorkshopChecks/input-fix.*.
"""
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile


def disable_auto_upgrade(text):
    """Set the editor upgrade CVar without rewriting unrelated INI content."""
    newline = '\r\n' if '\r\n' in text else '\n'
    sections = list(re.finditer(r'^\[([^\r\n]+)\][^\r\n]*', text, re.MULTILINE))
    matches = [i for i, section in enumerate(sections) if section.group(1) == 'SystemSettings']
    setting = 'EnhancedInput.bEnableAutoUpgrade=0'
    if not matches:
        return text + ('' if text.endswith(newline) else newline) + newline + '[SystemSettings]' + newline + setting + newline
    if len(matches) != 1:
        raise ValueError('Duplicate SystemSettings sections. No files were changed.')
    index = matches[0]
    start = sections[index].end()
    end = sections[index + 1].start() if index + 1 < len(sections) else len(text)
    body = text[start:end]
    body, count = re.subn(r'^EnhancedInput\.bEnableAutoUpgrade=[^\r\n]*', setting, body, flags=re.MULTILINE)
    if count > 1:
        raise ValueError('Duplicate EnhancedInput.bEnableAutoUpgrade settings. No files were changed.')
    if not count:
        body += ('' if body.endswith(newline) else newline) + setting + newline
    return text[:start] + body + text[end:]


def plan_repair(root):
    project_path = root / 'Proxima.uproject'
    config_path = root / 'Config/DefaultInput.ini'
    engine_path = root / 'Config/DefaultEngine.ini'
    original_project = project_path.read_bytes()
    original_config = config_path.read_bytes()
    original_engine = engine_path.read_bytes()
    project = json.loads(original_project)
    config = original_config.decode('utf-8')
    engine = disable_auto_upgrade(original_engine.decode('utf-8'))
    classes = {
        'DefaultPlayerInputClass': '/Script/Engine.PlayerInput',
        'DefaultInputComponentClass': '/Script/Engine.InputComponent',
    }
    for key, value in classes.items():
        pattern = r'^' + re.escape(key) + r'=[^\r\n]*'
        config, count = re.subn(pattern, key + '=' + value, config, flags=re.MULTILINE)
        if count != 1:
            raise ValueError(f'Expected exactly one {key} entry; found {count}. No files were changed.')
    plugins = project.setdefault('Plugins', [])
    enhanced = [p for p in plugins if p.get('Name') == 'EnhancedInput']
    if len(enhanced) > 1:
        raise ValueError('Duplicate EnhancedInput plugin entries. No files were changed.')
    if enhanced:
        enhanced[0]['Enabled'] = False
    else:
        plugins.insert(0, {'Name': 'EnhancedInput', 'Enabled': False})
    project_bytes = original_project if project == json.loads(original_project) else (
        json.dumps(project, indent=4) + '\n').encode('utf-8')
    updates = [(project_path, original_project, project_bytes),
               (config_path, original_config, config.encode('utf-8')),
               (engine_path, original_engine, engine.encode('utf-8'))]
    updates = [(path, old, new) for path, old, new in updates if old != new]
    return updates


def repair(root):
    updates = plan_repair(root)
    if updates:
        backup_parent = root / 'Saved/WorkshopChecks'
        backup_parent.mkdir(parents=True, exist_ok=True)
        backup = Path(tempfile.mkdtemp(prefix='input-fix.', dir=backup_parent))
        for path, _, _ in updates:
            shutil.copy2(path, backup / path.name)
        for path, _, content in updates:
            path.write_bytes(content)
        print('Input configuration corrected. Originals saved in: ' + str(backup))
    else:
        print('Input configuration already correct; no files changed.')
    print('EnhancedInput disabled; automatic upgrade off; PlayerInput and InputComponent selected.')


if __name__ == '__main__':
    try:
        if len(sys.argv) > 2:
            raise ValueError('Usage: python3 repair_legacy_input.py [path/to/Proxima]')
        repair(Path(sys.argv[1] if len(sys.argv) == 2 else '.').resolve())
    except (OSError, ValueError, TypeError, KeyError, AttributeError) as error:
        print('INPUT REPAIR FAILED: ' + str(error), file=sys.stderr)
        sys.exit(1)
