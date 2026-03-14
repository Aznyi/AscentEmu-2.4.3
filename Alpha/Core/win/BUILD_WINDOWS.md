# Windows Build Guide

This guide covers the current first-time Windows build flow for the modernized AscentEmu solution.

The supported server layout is a single-realm deployment built from `ascent-world` and `ascent-logonserver`. Legacy `realmserver`/clustering support is intentionally retired and is not part of the current Windows solution.

## Recommended Host Setup

- Visual Studio 2026 or newer with Desktop development for C++
- MSVC toolchain with the `v145` platform toolset installed
- Windows 10 or newer SDK
- PowerShell

The primary supported target is `x64`. `Win32` also builds, but `x64` should be the default for new setups.

## First-Time Setup

1. Clone the repository.
2. Open PowerShell at the repository root.
3. Stage the external dependency payload into the repo-local `.deps` tree:

```powershell
.\Alpha\Core\win\bootstrap-deps.ps1 -Platform x64 -Source VendorFallback -Force
```

This creates a deterministic dependency layout under:

```text
Alpha/Core/.deps/
  include/
  lib/
    x64/
    Win32/
  bin/
    x64/
    Win32/
```

The bootstrap script currently stages:

- MySQL headers and `libmysql.lib`
- OpenSSL headers and `libeay32.lib`
- runtime DLLs when the repo fallback includes them
- a dependency manifest at `Alpha/Core/.deps/manifest.<platform>.json`

For a normal first build, `VendorFallback` is the intended starting point. `PinnedArchives` is supported when pinned dependency archives have been downloaded into `.deps/downloads`.

## Open The Solution

Open:

- [ascentVC90.sln](D:/Server/Github/Ascent-2.4.3/AscentEmu-2.4.3/Alpha/Core/win/ascentVC90.sln)

## Recommended First Build

Use one of these solution configurations:

- `Release|x64` for a runnable server build
- `Debug|x64` for active development and debugging

## Build Order

The solution dependency graph handles the important ordering automatically. The effective build order is:

1. `pcre`
2. `zlib`
3. `ascent-shared`
4. `ascent-world`
5. `ascent-logonserver`
6. script DLLs

The script DLL set currently includes:

- `GossipScripts`
- `InstanceScripts`
- `ServerStatusPlugin`
- `SpellHandlers`

`LUAScripting` is intentionally removed and is not part of the build.

## Output Locations

### x64

- `Debug|x64`: `Alpha/Core/bin/Debug_x64`
- `Release|x64`: `Alpha/Core/bin/Release_x64`

Script DLLs are placed under:

- `Alpha/Core/bin/Debug_x64/script_bin`
- `Alpha/Core/bin/Release_x64/script_bin`

### Win32

- `Debug|Win32`: `Alpha/Core/bin/Debug`
- `Release|Win32`: `Alpha/Core/bin/Release`

Script DLLs are placed under:

- `Alpha/Core/bin/Debug/script_bin`
- `Alpha/Core/bin/Release/script_bin`

## What Builds From Source

These dependencies are built directly from the repository by the solution:

- `pcre`
- `zlib`

These dependencies are staged into `.deps` before the build:

- MySQL client headers/import library
- OpenSSL headers/import library

## First Runtime Check

After a successful `Release|x64` build, the main binaries are in:

- [Release_x64](D:/Server/Github/Ascent-2.4.3/AscentEmu-2.4.3/Alpha/Core/bin/Release_x64)

The main executables are:

- `ascent-world.exe`
- `ascent-logonserver.exe`

They expect configuration files and runtime dependency DLLs to be present in the output directory.

## Troubleshooting

- If Visual Studio cannot find MySQL or OpenSSL headers/libs, rerun `bootstrap-deps.ps1`.
- If you are building for the first time, prefer `x64` before trying `Win32`.
- If you use `PinnedArchives`, make sure the required archives exist in `.deps/downloads` or let the script fetch them.
- If runtime startup fails because a DLL is missing, check `Alpha/Core/.deps/bin/<platform>` and the target output directory.

## Current Known Gaps

- The repo fallback currently contains x64 runtime DLLs, but not matching Win32 runtime DLLs.
- The pinned archive path is supported by the bootstrap script, but the repo fallback remains the most validated first-time setup path.
