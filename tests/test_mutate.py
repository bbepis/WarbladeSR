#!/usr/bin/env python3
"""Unit checks for mutation-runner failure handling; no game build is needed."""

import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).with_name("mutate.py")
spec = importlib.util.spec_from_file_location("warblade_mutate", SCRIPT)
mutate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mutate)


class MutationRunner(unittest.TestCase):
    def setUp(self):
        self.old_worker = mutate._worker
        self.old_error = mutate._worker_error
        mutate._worker_error = None

    def tearDown(self):
        mutate._worker = self.old_worker
        mutate._worker_error = self.old_error

    def mutation(self, expected=()):
        m = mutate.Mutation("probe", "probe.txt:1")
        m.expect = list(expected)
        return m

    def test_nonzero_without_fail_is_runner_error(self):
        m = self.mutation()
        mutate._worker = mock.Mock()
        mutate._worker.mutate.return_value = ("fail", "0 tests, 0 failed\n")
        self.assertEqual(mutate.check(m)[1], "runner")
        mutate._worker.mutate.return_value = ("fail", "")
        self.assertEqual(mutate.check(m)[1], "runner")

    def test_recorded_fail_still_needs_expected_test(self):
        m = self.mutation(["expected_probe"])
        mutate._worker = mock.Mock()
        mutate._worker.mutate.return_value = ("fail", "FAIL other_probe (probe.c)\n")
        self.assertEqual(mutate.check(m)[1], "missed")
        mutate._worker.mutate.return_value = ("fail", "FAIL expected_probe (probe.c)\n")
        self.assertEqual(mutate.check(m)[1], "killed")

    def test_worker_runs_expected_subset_and_detects_runner_timeout(self):
        worker = mutate.Worker.__new__(mutate.Worker)
        worker.dir = "/unused"
        worker.build = "/unused/build"
        with mock.patch.object(mutate, "run", side_effect=[(0, ""), (1, "FAIL expected_probe\n")]) as run, \
             mock.patch.object(mutate.os.path, "exists", return_value=True):
            self.assertEqual(worker.test(False, ["expected_probe"])[0], "fail")
            self.assertEqual(run.call_args_list[1].args[0][-1], "expected_probe")
        with mock.patch.object(mutate, "run", side_effect=[(0, ""), (-1, "timed out")]), \
             mock.patch.object(mutate.os.path, "exists", return_value=True):
            self.assertEqual(worker.test(False)[0], "runner")
        with mock.patch.object(mutate, "run", side_effect=[(0, ""), (2, "wait failed")]), \
             mock.patch.object(mutate.os.path, "exists", return_value=True):
            self.assertEqual(worker.test(False)[0], "runner")

    def test_baseline_uses_full_suite_and_cleans_on_failure(self):
        parents = []
        calls = []

        class FailingBaseline:
            def __init__(self, parent):
                parents.append(parent)

            def test(self, engine, *patterns):
                calls.append((engine, patterns))
                return "fail", "baseline failure"

        with mock.patch.object(mutate, "Worker", FailingBaseline), \
             mock.patch.object(mutate, "parse", return_value=[]), \
             mock.patch.object(sys, "argv", [str(SCRIPT)]):
            with self.assertRaises(SystemExit):
                mutate.main()
        self.assertEqual(len(parents), 1)
        self.assertEqual(calls, [(False, ())])
        self.assertFalse(os.path.exists(parents[0]))

    def test_partial_worker_and_baseline_configure_cleanup(self):
        with tempfile.TemporaryDirectory(prefix="wb-mutate-unit-") as parent:
            with mock.patch.object(mutate.shutil, "copytree", side_effect=OSError("copy failed")):
                with self.assertRaises(OSError):
                    mutate.Worker(parent)
            self.assertEqual(os.listdir(parent), [])

        parents = []

        def fail_configure(parent):
            parents.append(parent)
            raise RuntimeError("configure failed")

        with mock.patch.object(mutate, "Worker", side_effect=fail_configure), \
             mock.patch.object(mutate, "parse", return_value=[]), \
             mock.patch.object(sys, "argv", [str(SCRIPT)]):
            with self.assertRaises(RuntimeError):
                mutate.main()
        self.assertFalse(os.path.exists(parents[0]))

    def test_initializer_reports_error_without_pool_respawn(self):
        m = self.mutation()
        with mock.patch.object(mutate, "Worker", side_effect=RuntimeError("configure failed")) as worker:
            mutate.init_worker("/unused")
            self.assertEqual(mutate.check(m)[1], "build")
            self.assertIn("configure failed", mutate.check(m)[2])
            worker.assert_called_once_with("/unused")


if __name__ == "__main__":
    unittest.main()
