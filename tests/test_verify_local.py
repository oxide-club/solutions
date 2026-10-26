# SPDX-License-Identifier: MIT
"""O verificador respeita o catálogo sem inventar CLIs para bibliotecas."""

import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location("verify_local", TOOLS / "verify_local.py")
verify = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify)


class VerificationRouting(unittest.TestCase):
    def exercise(self, passed=True, make_failure=False):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        entries = [
            {"challenge": "001-wc", "language": "c"},
            {"challenge": "022-typestate", "language": "rust"},
        ]
        for entry in entries:
            (root / entry["challenge"] / entry["language"]).mkdir(parents=True)
        calls = []

        def run(command, cwd):
            calls.append(command)
            if "list" in command:
                return json.dumps({"challenges": [{"id": "001-wc"}]})
            if command[0] == "make":
                if make_failure and Path(cwd).parent.name == "022-typestate":
                    raise RuntimeError("suíte própria falhou")
                return ""
            return json.dumps({"all_public_tests_passed": passed, "summary": {"passed": 3}})

        with patch.object(verify, "ROOT", root), \
             patch.object(verify, "discover", return_value={"include": entries}), \
             patch.object(verify, "run", side_effect=run), \
             patch.object(sys, "argv", ["verify_local.py", "--runner", str(root / "runner")]), \
             contextlib.redirect_stdout(io.StringIO()), \
             contextlib.redirect_stderr(io.StringIO()):
            result = verify.main()
        return result, calls

    def test_every_reference_runs_make_but_only_catalogued_program_runs_runner(self):
        result, calls = self.exercise()
        self.assertEqual(result, 0)
        self.assertEqual(sum(call[0] == "make" for call in calls), 2)
        runner_calls = [call for call in calls if "run" in call]
        self.assertEqual(len(runner_calls), 1)
        self.assertEqual(runner_calls[0][2], "001-wc")

    def test_incomplete_runner_suite_fails(self):
        result, _ = self.exercise(passed=False)
        self.assertEqual(result, 1)

    def test_library_make_failure_is_not_hidden_by_absence_from_runner(self):
        result, _ = self.exercise(make_failure=True)
        self.assertEqual(result, 1)
