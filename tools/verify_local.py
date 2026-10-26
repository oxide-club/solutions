#!/usr/bin/env python3
"""Verify both local checkouts while developing changes not yet in the lock file."""

import argparse
import json
from pathlib import Path
import subprocess
import sys

from list_solutions import discover

ROOT = Path(__file__).resolve().parents[1]


def run(command, cwd):
    result = subprocess.run(command, cwd=cwd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    if result.returncode:
        print(result.stdout, file=sys.stderr)
        raise RuntimeError(f"falhou ({result.returncode}): {' '.join(map(str, command))}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--challenges-dir", type=Path, default=ROOT.parent / "challenges")
    parser.add_argument("--runner", type=Path, help="oxide-test já compilado; padrão: compilar o checkout")
    parser.add_argument("--challenge", action="append", help="limitar aos IDs indicados (repetível)")
    args = parser.parse_args()
    challenges = args.challenges_dir.resolve()
    try:
        entries = discover(ROOT)["include"]
        if args.challenge:
            unknown = set(args.challenge) - {entry["challenge"] for entry in entries}
            if unknown:
                raise ValueError(f"desafios desconhecidos: {', '.join(sorted(unknown))}")
            entries = [entry for entry in entries if entry["challenge"] in args.challenge]
        if args.runner:
            runner = args.runner.resolve()
        else:
            run(["cargo", "build", "--manifest-path", "tools/oxide-test/Cargo.toml", "--locked"], challenges)
            runner = challenges / "tools/oxide-test/target/debug/oxide-test"
        # Bibliotecas, shared objects e sessões têm suítes próprias no Makefile.
        # O catálogo do runner determina quais referências também aceitam sua CLI.
        catalog = json.loads(run([str(runner), "list", "--json"], ROOT))
        runner_ids = {entry["id"] for entry in catalog["challenges"]}
        print("Verificação local: usa os arquivos atuais; não altera challenges.lock.json.", flush=True)
        total = 0
        runner_count = 0
        for entry in entries:
            identifier, language = entry["challenge"], entry["language"]
            directory = ROOT / identifier / language
            # The supplied adapters are copied to keep references buildable on
            # their own. A drift must not silently change the tested interface.
            for local in directory.iterdir():
                original = challenges / identifier / "materials" / local.name
                if local.suffix in (".c", ".h") and original.is_file():
                    if local.read_bytes() != original.read_bytes():
                        raise ValueError(f"material divergente: {local} e {original}")
            run(["make", "test", f"CHALLENGES_DIR={challenges}"], directory)
            if identifier not in runner_ids:
                print(f"OK {identifier}/{language}: suíte própria do desafio", flush=True)
                continue
            executable = directory / "build" / f"oxide-{identifier.split('-', 1)[1]}"
            report = json.loads(run([str(runner), "run", identifier, str(executable), "--json"], ROOT))
            if report.get("all_public_tests_passed") is not True:
                raise RuntimeError(f"suíte incompleta: {identifier}/{language}")
            count = report["summary"]["passed"]
            total += count
            runner_count += 1
            print(f"OK {identifier}/{language}: Python + API (quando aplicável), Rust {count} casos", flush=True)
        print(f"{len(entries)} referências verificadas; {runner_count} também no runner Rust ({total} casos).")
        return 0
    except (OSError, ValueError, RuntimeError) as error:
        print(error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
