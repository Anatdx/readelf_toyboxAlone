# readelf_toyboxAlone

This directory contains the source needed to build the single-command Toybox
`readelf` applet used by ksud.

- Upstream: <https://github.com/landley/toybox>
- Android 16 upstream commit: `2cc5e25fb107fe0ff77c95a983474497b76ac9f8`
- AOSP baseline: `android-16.0.0_r1` / Toybox 0.8.12 generation
- License: 0BSD; see `LICENSE`

The snapshot contains upstream `main.c`, `toys.h`, `lib/*`,
`toys/other/readelf.c`, and the generated headers for an `allnoconfig` build
with only `CONFIG_READELF=y` enabled. The corresponding Kconfig output is
stored in `readelf.config`.

No ELF parsing code is maintained by YukiSU. The CMake target compiles the
upstream sources into a private static archive. Every symbol defined by that
archive is rewritten with the `yukisu_toybox_` prefix before linking,
preventing collisions with the BusyBox implementation already linked into
ksud. `ksud readelf` dispatches to the namespaced Toybox entry point.
