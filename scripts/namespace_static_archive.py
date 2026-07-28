#!/usr/bin/env python3
"""Namespace every externally defined symbol in a static archive.

The matching undefined references inside the archive are rewritten by
llvm-objcopy as well. Undefined libc symbols are not present in the map and
therefore keep their original names.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


SYMBOL_PATTERN = re.compile(r"^(\S+)\s+[A-Za-z]\s")


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nm", required=True)
    parser.add_argument("--objcopy", required=True)
    parser.add_argument("--ranlib", required=True)
    parser.add_argument("--prefix", required=True)
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    source = Path(args.input).resolve()
    output = Path(args.output).resolve()
    if not source.is_file():
        raise FileNotFoundError(f"input archive does not exist: {source}")
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", args.prefix):
        raise ValueError(f"invalid symbol prefix: {args.prefix!r}")

    nm = run(
        [
            args.nm,
            "-g",
            "--defined-only",
            "--format=posix",
            str(source),
        ]
    )
    symbols = sorted(
        {
            match.group(1)
            for line in nm.stdout.splitlines()
            if (match := SYMBOL_PATTERN.match(line))
        }
    )
    required = {"main", "readelf_main"}
    missing = sorted(required.difference(symbols))
    if missing:
        raise RuntimeError(
            f"archive is missing required Toybox symbols: {', '.join(missing)}"
        )

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent) as temporary_directory:
        temporary = Path(temporary_directory)
        symbol_map = temporary / "symbols.map"
        namespaced_archive = temporary / output.name
        symbol_map.write_text(
            "".join(
                f"{symbol} {args.prefix}{symbol}\n"
                for symbol in symbols
            ),
            encoding="utf-8",
            newline="\n",
        )
        run(
            [
                args.objcopy,
                f"--redefine-syms={symbol_map}",
                str(source),
                str(namespaced_archive),
            ]
        )
        run([args.ranlib, str(namespaced_archive)])
        os.replace(namespaced_archive, output)

    print(
        f"Namespaced {len(symbols)} symbols in {source.name} "
        f"with prefix {args.prefix}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
