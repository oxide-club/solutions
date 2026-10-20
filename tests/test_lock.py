import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class RevisionLock(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="lock test ")
        self.addCleanup(self.directory.cleanup)
        self.work = Path(self.directory.name)
        self.checkout = self.work / "challenges"
        self.checkout.mkdir()
        self.git("init", "-q")
        (self.checkout / "example").write_text("public tests\n")
        self.git("add", "example")
        self.git("-c", "user.name=Test", "-c", "user.email=test@example.invalid",
                 "commit", "-qm", "initial")
        self.revision = self.git("rev-parse", "HEAD").stdout.strip()
        self.manifest = self.work / "lock.json"
        self.write_manifest()

    def git(self, *args):
        return subprocess.run(
            ["git", "-C", str(self.checkout), *args],
            capture_output=True, text=True, check=True,
        )

    def write_manifest(self, **updates):
        data = {"repository": "oxide-club/challenges", "revision": self.revision}
        data.update(updates)
        self.manifest.write_text(json.dumps(data))

    def check(self, *args):
        return subprocess.run(
            [sys.executable, str(ROOT / "tools/check_lock.py"),
             "--manifest", str(self.manifest), *map(str, args)],
            capture_output=True, text=True,
        )

    def test_matching_clean_checkout_passes(self):
        result = self.check("--checkout", self.checkout)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_wrong_commit_fails(self):
        self.write_manifest(revision="0" * 40)
        result = self.check("--checkout", self.checkout)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("revisão", result.stderr)

    def test_modified_test_file_fails(self):
        (self.checkout / "example").write_text("changed tests\n")
        result = self.check("--checkout", self.checkout)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("alterações", result.stderr)

    def test_untracked_test_file_fails(self):
        (self.checkout / "extra.py").write_text("pass\n")
        result = self.check("--checkout", self.checkout)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("alterações", result.stderr)

    def test_nested_directory_is_not_a_checkout(self):
        nested = self.checkout / "nested"
        nested.mkdir()
        result = self.check("--checkout", nested)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("raiz", result.stderr)

    def test_branch_names_and_invalid_sha_fail(self):
        for revision in ("main", "abc123", "g" * 40, "a" * 40 + "\nother=value"):
            with self.subTest(revision=revision):
                self.write_manifest(revision=revision)
                result = self.check()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("SHA", result.stderr)

    def test_invalid_repository_fails_without_writing_outputs(self):
        output = self.work / "github-output"
        self.write_manifest(repository="oxide-club/challenges\nref=main")
        result = self.check("--github-output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("repository", result.stderr)
        self.assertFalse(output.exists())

    def test_ci_outputs_are_validated(self):
        output = self.work / "github-output"
        result = self.check("--github-output", output)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            output.read_text(),
            f"repository=oxide-club/challenges\nrevision={self.revision}\n",
        )

    def test_invalid_json_and_missing_fields_fail(self):
        for content in ("{", "[]", "{}", '{"repository": null, "revision": 42}'):
            with self.subTest(content=content):
                self.manifest.write_text(content)
                result = self.check()
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
