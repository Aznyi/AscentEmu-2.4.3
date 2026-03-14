# Windows Dependency Decisions

This document records how the Windows build obtains every dependency that the modernized Visual Studio solution touches.

The dependency decisions below apply to the supported single-realm server layout built from `ascent-world` and `ascent-logonserver`. Retired `realmserver`/clustering components are out of scope for this build.

For the first-time Visual Studio setup and build flow, see `BUILD_WINDOWS.md` in this directory.

## Decisions

| Dependency | Decision | Current build path | Notes |
| --- | --- | --- | --- |
| `pcre` | `build-from-source` | `Alpha/Core/extras/Sources/VC90-pcre.vcxproj` | Built from the repository and emitted into `Alpha/Core/.deps/lib/<platform>/pcre.lib`. Headers come from `Alpha/Core/extras/Sources/pcre`. |
| `zlib` | `build-from-source` | `Alpha/Core/extras/Sources/VC90-zlib.vcxproj` | Built from the repository and emitted into `Alpha/Core/.deps/lib/<platform>/zlib.lib`. Headers come from `Alpha/Core/extras/Sources/zlib`. |
| `MariaDB Connector/C` (`libmysql`) | `pin externally` | `Alpha/Core/win/bootstrap-deps.ps1` | Preferred replacement for the legacy ad hoc MySQL client drop. The bootstrap can stage it from a pinned archive when one is present under `Alpha/Core/.deps/downloads`, and normalizes the import library name to `libmysql.lib`. |
| `OpenSSL` (`libcrypto`) | `pin externally` | `Alpha/Core/win/bootstrap-deps.ps1` | Preferred replacement for the legacy `libeay32` drop. The bootstrap can stage it from a pinned archive when one includes both headers and a Windows import library, and normalizes the staged import library name to `libeay32.lib`. |
| Legacy `Alpha/Core/src/dep` headers/libs | `vendor` fallback | `Alpha/Core/win/bootstrap-deps.ps1` | Used as the deterministic repo-local fallback when pinned archives are not available yet. The bootstrap stages the checked-in headers and libraries into `Alpha/Core/.deps`, and records the result in a manifest. |
| `dbghelp`, `ws2_32` | `system` | Windows SDK | Standard Windows SDK libraries provided by the installed Visual Studio toolchain. |
| Unused legacy `dep\include`/`dep\src` project entries | `remove` | Project cleanup | Old relative include paths that do not map to a maintained dependency layout are removed from touched project files. |

## Bootstrap Output Layout

The Windows build expects deterministic staged dependencies here:

```text
Alpha/Core/.deps/
  include/
  lib/
    Win32/
    x64/
```

`pcre` and `zlib` are built directly into `.deps/lib/<platform>`. Pinned external dependencies are staged into the same tree by `bootstrap-deps.ps1`.

Runtime DLLs are staged under:

```text
Alpha/Core/.deps/bin/
  Win32/
  x64/
```

`bootstrap-deps.ps1` also writes `Alpha/Core/.deps/manifest.<platform>.json` so each staged dependency set records the source mode, version pins, and file hashes.

## Bootstrap Modes

- `Auto`: use pinned archives if they are already present under `.deps/downloads`, otherwise stage the checked-in repo fallback.
- `VendorFallback`: copy the checked-in MySQL/OpenSSL headers and import libraries from `Alpha/Core/src/dep` into `.deps`.
- `PinnedArchives`: download or use pre-downloaded pinned archives, extract them, and stage a normalized `.deps` layout.

The current repository fallback is enough to compile all four Windows solution configurations. The pinned-archive path remains the preferred long-term source for `libmysql` and `libeay32` replacements.

At the moment, the checked-in fallback payload contains x64 runtime DLLs but not the matching Win32 DLLs. `bootstrap-deps.ps1` records that gap in the per-platform manifest and warns when Win32 runtime deployment still needs manual input.

## Version Pins

- `MariaDB Connector/C`: `3.3.10`
- `OpenSSL`: `1.1.1w`
- `pcre`: vendored source snapshot in `extras/Sources/pcre`
- `zlib`: vendored source snapshot in `extras/Sources/zlib`

These pins are intentionally explicit so the dependency acquisition path stays reproducible across machines and CI environments.
