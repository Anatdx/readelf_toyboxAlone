#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def run_readelf(executable: Path, elf_file: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(executable), "-h", str(elf_file)],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=5,
    )


def synthetic_elf64() -> bytes:
    image = bytearray(256)
    ident = b"\x7fELF\x02\x01\x01" + bytes(9)
    struct.pack_into(
        "<16sHHIQQQIHHHHHH",
        image,
        0,
        ident,
        1,  # ET_REL
        183,  # AArch64
        1,
        0xFEDCBA9876543210,
        64,
        120,
        0,
        64,
        56,
        1,
        64,
        2,
        0,
    )
    return bytes(image)


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} READELF ELF_FILE")

    executable = Path(sys.argv[1])
    elf_file = Path(sys.argv[2])
    result = run_readelf(executable, elf_file)
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

    with tempfile.TemporaryDirectory() as directory:
        high_entry = Path(directory) / "high-entry.elf"
        high_entry.write_bytes(synthetic_elf64())
        result = run_readelf(executable, high_entry)
        if result.returncode != 0 or "0xfedcba9876543210" not in result.stdout:
            sys.stderr.write(result.stdout)
            sys.stderr.write(result.stderr)
            return 1

        extended = bytearray(synthetic_elf64())
        struct.pack_into("<H", extended, 56, 0xFFFF)
        struct.pack_into("<I", extended, 120 + 44, 1)
        extended_file = Path(directory) / "extended-phnum.elf"
        extended_file.write_bytes(extended)
        result = run_readelf(executable, extended_file)
        if (
            result.returncode != 0
            or "Number of program headers:         1" not in result.stdout
        ):
            sys.stderr.write(result.stdout)
            sys.stderr.write(result.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
