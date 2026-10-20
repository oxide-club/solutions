#!/usr/bin/env python3
"""Discover per-language Makefiles for the CI matrix."""

import json
from pathlib import Path
import re
import sys


def discover(root):
    entries = []
    for challenge in sorted(root.iterdir()):
        if not challenge.is_dir() or not challenge.name[:1].isdigit():
            continue
        if not re.fullmatch(r"(?!000-)[0-9]{3}-[a-z0-9]+(?:-[a-z0-9]+)*", challenge.name):
            raise ValueError(f"identificador inválido: {challenge.name}")
        languages = [path for path in sorted(challenge.iterdir()) if path.is_dir()]
        if not languages:
            raise ValueError(f"Nenhuma implementação em {challenge.name}")
        for language in languages:
            if not re.fullmatch(r"[a-z][a-z0-9-]*", language.name):
                raise ValueError(f"Diretório de linguagem inválido: {language.name}")
            if not (language / "Makefile").is_file():
                raise ValueError(f"Makefile ausente: {challenge.name}/{language.name}")
            entries.append({"challenge": challenge.name, "language": language.name})
    if not entries:
        raise ValueError("Nenhuma implementação encontrada")
    return {"include": entries}


if __name__ == "__main__":
    try:
        root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
        print(json.dumps(discover(root)))
    except (ValueError, OSError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
