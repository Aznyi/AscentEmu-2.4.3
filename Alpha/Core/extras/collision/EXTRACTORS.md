# Terrain / VMap / MMap Extractor Notes

This branch contains a real legacy TBC vmap pipeline and now also has an in-tree mmap bake toolchain that can generate runtime-loadable mmap output from Ascent vmaps for verified test cases.

What actually exists in the tree:

- `Alpha/Core/extras/collision/collision_dll/collision.vcxproj`
- `Alpha/Core/extras/collision/extractor/vmapExtractor_VC90.vcproj`
- `Alpha/Core/extras/collision/assembler/vmap_assembler_vc9.vcproj`
- `Alpha/Core/extras/collision/extractor/vmapExtractor.vcxproj`
- `Alpha/Core/extras/collision/assembler/vmap_assembler.vcxproj`
- `Alpha/Core/extras/collision/extractor/Run-VMap-Pipeline.ps1`
- `Alpha/Core/extras/mmap/generator/movemapgen.vcxproj`

What changed in this pass:

- A modern build wrapper for the existing runtime collision library was added.
- Runtime collision loading/logging was audited and tightened.
- The extractor pipeline was documented and wrapped for repeatable use.
- Official Recast/Detour sources are now vendored in-tree for runtime/query work.
- CMaNGOS-derived mmap shared-format and generator-side sources are now staged in-tree under `Alpha/Core/extras/mmap/`.
- An Ascent-specific vmap mesh bridge now feeds those imported generator sources from extracted `.vmdir` / `.vmap` data.
- The in-tree `movemapgen` build is now working and has been verified to produce `036.mmap` and `036_33_32.mmtile` for a Deadminies test tile from WoW TBC `2.4.3.8606` client assets.
- The underlying legacy vmap extractor and assembler sources were not heavily rewritten here.

What does not exist yet:

- No claim should be made that broad/full-client mmap generation has been verified yet
- Outdoor/liquid-aware mmap baking still depends on extracted `maps/` data being present
- Real routed gameplay validation still needs more in-server testing across more maps
- See `Alpha/Core/extras/mmap/MMAP_IMPORTS.md` for the exact imported backend/generator source origins

For WoW TBC client `2.4.3.8606`, the verified fresh extraction path is:

- vmaps: fully verified
- mmaps: verified for at least one real indoor test tile using vmap-only source geometry

## Build

1. Build `Alpha/Core/extras/collision/collision_dll/collision.vcxproj`.
   The verified x64 release outputs are:
   - `Alpha/Core/extras/collision/collision_dll/x64/Release/collision.dll`
   - `Alpha/Core/extras/collision/collision_dll/x64/Release/collision.lib`
   The project also copies `collision.dll` into `Alpha/Core/bin/Release_x64/` after a successful x64 release build.
1. Build `Alpha/Core/extras/collision/extractor/vmapExtractor.vcxproj`.
   The default release output is `Alpha/Core/extras/collision/extractor/bin/Win32/ReleaseAS/vmapextract_v2.exe`.
2. Build `Alpha/Core/extras/collision/assembler/vmap_assembler.vcxproj`.
   The default release output is `Alpha/Core/extras/collision/assembler/Release/vmap_assembler.exe`.
3. Build `Alpha/Core/extras/mmap/generator/movemapgen.vcxproj`.
   The verified release output is `Alpha/Core/extras/mmap/generator/bin/Win32/Release/movemapgen.exe`.

Legacy VC90 project files are still present as source-era references, but the verified build path on this branch is the `.vcxproj` wrappers above.

## Use

From the WoW TBC `2.4.3.8606` client directory:

1. Run `vmapextract_v2.exe`.
   This creates the raw `Buildings` output used by the assembler.
2. Run `vmap_assembler.exe Buildings vmaps`.
   This creates the runtime `vmaps` directory.

Or use the wrapper from this repo:

```powershell
powershell -ExecutionPolicy Bypass -File Alpha\Core\extras\collision\extractor\Run-VMap-Pipeline.ps1 -ClientPath "D:\Games\WoW-TBC-2.4.3.8606" -OutputPath ".\vmaps"
```

To generate mmaps after extraction:

1. Ensure the client working folder contains `vmaps/`.
2. If you also have `maps/`, place them beside `vmaps/` so terrain/liquid can be included.
3. Run `movemapgen.exe`.

Examples:

```powershell
Alpha\Core\extras\mmap\generator\bin\Win32\Release\movemapgen.exe 36 --tile 32,33 --workdir "C:\Users\colby\Downloads\WoW TBC 2.4.3.8606" --threads 1
```

```powershell
Alpha\Core\extras\mmap\generator\bin\Win32\Release\movemapgen.exe 36 --workdir "C:\Users\colby\Downloads\WoW TBC 2.4.3.8606" --threads 1
```

Expected mmap outputs:

- `mmaps/%03u.mmap`
- `mmaps/%03u_%02u_%02u.mmtile`

## Runtime Layout Agreement

The current runtime expects:

- `maps/Map_<id>.bin`
- `vmaps/<files produced by vmap_assembler>`

The current vmap runtime loads directory manifests named:

- `%03u_%d_%d.vmdir` for tiled map data
- `%03u.vmdir` for non-tiled map data

Those `.vmdir` files reference `.vmap` model files under the same `vmaps` base path. This matches the legacy extractor + assembler pipeline already present in this tree.

## Runtime Config

To test vmaps on this branch:

```xml
<Terrain MapPath = "maps"
         vMapPath = "vmaps"
         UnloadMaps = "1"
         CollisionLogTileLoads = "1"
         CollisionStartupProbe = "1"
         CollisionDebugGroundZ = "0"
         CollisionDebugMovement = "0"
         CreatureGroundMovementThreshold = "10">
```

Notes:

- `CollisionStartupProbe = 1` performs one real `loadMap` / `unloadMap` against the extracted `vmaps` tree during server startup and logs the exact tile file used.
- Runtime tile activation now logs the requested vmap file name and load result when `CollisionLogTileLoads = 1`.
- Runtime logging now distinguishes a missing `vMapPath` root, a missing tile manifest (`.vmdir`), and a manifest that exists but still fails to load.
- VMap floor height is preferred over terrain for indoor floors, bridges, caves, and WMO interiors.
- The runtime now contains a real Detour-backed navmesh query backend behind `MMapInterface`.
- The server still falls back to the validated direct-movement path when mmap data is absent or a navmesh query fails.
- Future mmap data is expected under `mmaps/` named `%03u.mmap` and `%03u_%02u_%02u.mmtile`.
- The generator-side sources for producing those files are now in-tree and build as `movemapgen.exe`.
- Verified bake example:
  - input workdir: `C:\Users\colby\Downloads\WoW TBC 2.4.3.8606`
  - output files: `mmaps\036.mmap` and `mmaps\036_33_32.mmtile`
- If `maps/` is missing, the generator logs that terrain/liquid input is being skipped; vmap-only indoor generation can still succeed where geometry is sufficient.
