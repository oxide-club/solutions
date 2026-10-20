import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SolutionDiscovery(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def solution(self, challenge, language, makefile=True):
        directory = self.root / challenge / language
        directory.mkdir(parents=True)
        if makefile:
            (directory / "Makefile").write_text("check:\n\ttrue\n")

    def discover(self):
        return subprocess.run(
            [sys.executable, str(ROOT / "tools/list_solutions.py"), str(self.root)],
            capture_output=True, text=True,
        )

    def test_all_languages_are_included_in_sorted_ci_matrix(self):
        self.solution("002-ppm", "c")
        self.solution("001-wc", "rust")
        self.solution("001-wc", "c")
        result = self.discover()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout), {"include": [
            {"challenge": "001-wc", "language": "c"},
            {"challenge": "001-wc", "language": "rust"},
            {"challenge": "002-ppm", "language": "c"},
        ]})

    def test_missing_makefile_is_not_silently_skipped(self):
        self.solution("001-wc", "c", makefile=False)
        result = self.discover()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Makefile", result.stderr)

    def test_invalid_language_fails(self):
        self.solution("001-wc", "C++")
        result = self.discover()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("linguagem", result.stderr)

    def test_empty_repository_fails(self):
        result = self.discover()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Nenhuma", result.stderr)

    def test_invalid_numbered_challenge_fails(self):
        self.solution("01-wc", "c")
        result = self.discover()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("identificador", result.stderr)


if __name__ == "__main__":
    unittest.main()
