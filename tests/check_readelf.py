#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import subprocess
import sys


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} READELF ELF_FILE")

    executable = Path(sys.argv[1])
    elf_file = Path(sys.argv[2])
    result = subprocess.run(
        [str(executable), "-h", str(elf_file)],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=5,
    )
    required = (
        "ELF Header:",
        "Class:",
        "Data:",
        "Type:",
        "Machine:",
        "Section header string table index:",
    )
    if result.returncode != 0 or any(item not in result.stdout for item in required):
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
