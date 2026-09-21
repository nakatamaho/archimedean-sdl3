#!/usr/bin/env python3
"""Exercise negative cases for the independent CPython JSON validator."""

from __future__ import annotations

import copy
import json
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Callable

from validate_archimedean import load_document, validate_document


def _run_case(
    name: str,
    source: dict,
    mutate: Callable[[dict], None],
    validator: Path,
    directory: Path,
) -> None:
    document = copy.deepcopy(source)
    mutate(document)
    path = directory / f"{name}.json"
    with path.open("w", encoding="utf-8") as stream:
        json.dump(document, stream, allow_nan=True)
    process = subprocess.run(
        [sys.executable, str(validator), str(path)],
        capture_output=True,
        text=True,
        check=False,
    )
    if process.returncode == 0:
        raise AssertionError(f"{name}: corrupt input was accepted")
    print(f"PASS: rejected {name}: {process.stdout.strip()}")


def main() -> int:
    repository = Path(__file__).resolve().parents[1]
    source_path = repository / "data" / "archimedean.json"
    validator = repository / "tools" / "validate_archimedean.py"
    source = load_document(source_path)

    cases = [
        (
            "bad-index",
            lambda document: document["solids"][0]["faces"][0].__setitem__(
                0, len(document["solids"][0]["vertices"])
            ),
        ),
        (
            "missing-manifold-edge",
            lambda document: document["solids"][1]["edges"].pop(),
        ),
        (
            "wrong-count",
            lambda document: document["solids"][2]["statistics"].__setitem__(
                "vertices", 999
            ),
        ),
        (
            "nan-coordinate",
            lambda document: document["solids"][3]["vertices"][0].__setitem__(
                0, float("nan")
            ),
        ),
    ]

    with tempfile.TemporaryDirectory(prefix="archview-validator-") as temporary:
        directory = Path(temporary)
        for name, mutate in cases:
            _run_case(name, source, mutate, validator, directory)

    validate_document(source)
    print("PASS: valid canonical document remains accepted")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
