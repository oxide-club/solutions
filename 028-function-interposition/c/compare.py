#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compara a aplicação entregue com sua execução sem instrumentação."""

import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent


def run(name, mode, preload=False):
    env = os.environ.copy()
    env.pop("LD_PRELOAD", None)
    if preload:
        env["LD_PRELOAD"] = str(ROOT / "build/libtrace.so")
    result = subprocess.run(
        [str(ROOT / "build" / name), mode], env=env,
        text=True, capture_output=True, timeout=15, check=True,
    )
    return result.stdout, result.stderr


def main():
    baseline, errors = run("app", "healthy")
    assert not errors and len(baseline.splitlines()) == 240
    direct, errors = run("direct", "healthy", preload=True)
    assert (direct, errors) == (baseline, ""), "chamada direta"
    for mode in ("healthy", "errno", "fail", "reenter"):
        output, errors = run("app", mode, preload=True)
        assert output == baseline, f"resultado alterado: {mode}"
        records = [json.loads(line) for line in errors.splitlines()]
        assert len(records) == (0 if mode == "fail" else 240), mode
        assert len({row["call_id"] for row in records}) == len(records)
    print("PASS aplicação: baseline, quatro threads, sinks, reentrancy e chamada direta")


if __name__ == "__main__":
    main()
