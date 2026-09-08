#!/usr/bin/env python3
"""Require a complete, fresh Unreal automation report with all Proxima tests green."""
import argparse
import json
from pathlib import Path
import re
import sys


def expected_tests(source_root):
    tests = set()
    for path in source_root.rglob('*.cpp'):
        tests.update(re.findall(r'IMPLEMENT_SIMPLE_AUTOMATION_TEST\s*\(\s*\w+\s*,\s*"(Proxima\.[^"]+)"', path.read_text()))
    return tests


def check(report, source_root):
    payload = json.loads(report.read_text(encoding='utf-8-sig'))
    found = {}
    for test in payload.get('tests', []):
        name = test.get('fullTestPath', '')
        if name.startswith('Proxima.'):
            if name in found:
                raise ValueError('Duplicate test result: ' + name)
            found[name] = test.get('state', '')
    expected = expected_tests(source_root)
    if not expected:
        raise ValueError('No Proxima tests found in source')
    missing = sorted(expected - found.keys())
    bad = {name: state for name, state in found.items() if state != 'Success'}
    if missing or bad:
        raise ValueError(f'Missing tests: {missing}; non-success results: {bad}')
    if payload.get('failed', 0) or payload.get('notRun', 0) or payload.get('inProcess', 0):
        raise ValueError('Unreal report is incomplete or contains failures')
    return sorted(found)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    parser.add_argument('source_root', type=Path)
    args = parser.parse_args()
    try:
        passed = check(args.report, args.source_root)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print('AUTOMATION NOT VERIFIED: ' + str(exc), file=sys.stderr)
        return 1
    print(f'PASS: {len(passed)} Proxima Unreal automation tests; every authored test is present and successful.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
