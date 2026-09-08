#!/usr/bin/env python3
"""Tests the release gate, particularly false-green and incomplete UE reports."""
import json
from pathlib import Path
import tempfile
import unittest
from check_automation_report import check


class ReportGateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'Source'
        self.source.mkdir()
        (self.source / 'Tests.cpp').write_text('IMPLEMENT_SIMPLE_AUTOMATION_TEST(One, "Proxima.One", Flags)\n'
                                              'IMPLEMENT_SIMPLE_AUTOMATION_TEST(Two, "Proxima.Two", Flags)')
        self.report = self.root / 'index.json'

    def payload(self, tests, **summary):
        self.report.write_text(json.dumps({'tests': tests, **summary}))

    def test_complete_pass(self):
        self.payload([{'fullTestPath': 'Proxima.One', 'state': 'Success'},
                      {'fullTestPath': 'Proxima.Two', 'state': 'Success'}], failed=0, notRun=0)
        self.assertEqual(check(self.report, self.source), ['Proxima.One', 'Proxima.Two'])

    def test_missing_test(self):
        self.payload([{'fullTestPath': 'Proxima.One', 'state': 'Success'}])
        with self.assertRaises(ValueError):
            check(self.report, self.source)

    def test_failure_and_pending(self):
        for state in ['Fail', 'NotRun', 'InProcess', 'Skipped', '']:
            with self.subTest(state=state):
                self.payload([{'fullTestPath': 'Proxima.One', 'state': 'Success'},
                              {'fullTestPath': 'Proxima.Two', 'state': state}])
                with self.assertRaises(ValueError):
                    check(self.report, self.source)

    def test_empty_and_corrupt_report(self):
        self.payload([])
        with self.assertRaises(ValueError):
            check(self.report, self.source)
        self.report.write_text('{broken')
        with self.assertRaises(ValueError):
            check(self.report, self.source)

    def test_duplicate_result(self):
        self.payload([{'fullTestPath': 'Proxima.One', 'state': 'Success'}] * 2)
        with self.assertRaises(ValueError):
            check(self.report, self.source)

    def test_failure_summary(self):
        self.payload([{'fullTestPath': 'Proxima.One', 'state': 'Success'},
                      {'fullTestPath': 'Proxima.Two', 'state': 'Success'}], failed=1)
        with self.assertRaises(ValueError):
            check(self.report, self.source)


if __name__ == '__main__':
    unittest.main()
