#!/usr/bin/env python3
"""Validate the public-test revision before checkout or local execution."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def load_manifest(path):
    data = json.loads(path.read_text())
    if not isinstance(data, dict):
        raise ValueError("Manifesto deve ser um objeto JSON")
    repository = data.get("repository")
    revision = data.get("revision")
    if not isinstance(repository, str) or not re.fullmatch(
        r"[A-Za-z0-9][A-Za-z0-9_.-]*/[A-Za-z0-9][A-Za-z0-9_.-]*", repository
    ):
        raise ValueError("repository deve ter o formato organização/repositório")
    if not isinstance(revision, str) or not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("revision deve ser um SHA completo de 40 caracteres hexadecimais")
    return repository, revision


def check_checkout(directory, revision):
    directory = directory.resolve()

    def git(*args):
        result = subprocess.run(
            ["git", "-C", str(directory), *args], capture_output=True, text=True,
        )
        if result.returncode:
            raise ValueError(f"Não foi possível verificar o checkout: {result.stderr.strip()}")
        return result.stdout.strip()

    if Path(git("rev-parse", "--show-toplevel")).resolve() != directory:
        raise ValueError("CHALLENGES_DIR deve apontar para a raiz do checkout")
    if git("rev-parse", "HEAD") != revision:
        raise ValueError("Checkout usa outra revisão; consulte challenges.lock.json")
    if git("status", "--porcelain", "--untracked-files=all"):
        raise ValueError("Checkout contém alterações; use uma revisão limpa dos testes")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=ROOT / "challenges.lock.json")
    parser.add_argument("--checkout", type=Path)
    parser.add_argument("--github-output", type=Path)
    args = parser.parse_args()
    try:
        repository, revision = load_manifest(args.manifest)
        if args.checkout is not None:
            check_checkout(args.checkout, revision)
        if args.github_output is not None:
            with args.github_output.open("a") as output:
                output.write(f"repository={repository}\nrevision={revision}\n")
        print(f"Testes públicos: {repository}@{revision}")
        return 0
    except (ValueError, OSError) as error:
        print(error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
