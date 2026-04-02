# MMap Source Imports

This branch now vendors the minimum upstream source needed to begin real mmap work without replacing Ascent's movement layer.

## Imported components

Official RecastNavigation:

- Source: upstream `recastnavigation`
- Imported from:
  - `Recast/Include`
  - `Recast/Source`
  - `Detour/Include`
  - `Detour/Source`
- Vendored under:
  - `Alpha/Core/extras/third_party/recastnavigation`

CMaNGOS TBC references adapted for Ascent:

- Shared mmap file-format definitions:
  - `Alpha/Core/extras/mmap/runtime/MoveMapSharedDefines.h`
- Generator-side source staging:
  - `Alpha/Core/extras/mmap/generator/src`
  - `Alpha/Core/extras/mmap/generator/include`
  - `Alpha/Core/extras/mmap/generator/third_party/json/json.hpp`

Attribution files copied in-tree:

- `Alpha/Core/extras/third_party/recastnavigation/License.txt`
- `Alpha/Core/extras/mmap/LICENSE`
- `Alpha/Core/extras/mmap/AUTHORS.md`

## Current integration status

Implemented now:

- Detour runtime sources are compiled into `ascent-world`
- `MMapInterface` now loads real `.mmap` / `.mmtile` data and issues Detour path queries
- AI still uses the Ascent-native path abstraction and safe direct fallback
- An Ascent-specific vmap bridge feeds extracted `.vmdir` / `.vmap` geometry into the imported bake pipeline
- `movemapgen` now builds in-tree and has been verified to write at least one real `%03u.mmap` / `%03u_%02u_%02u.mmtile` pair

Not complete yet:

- Full-world/full-client mmap generation has not been exhaustively verified
- Outdoor/liquid-aware baking still depends on extracted `maps/` data being present
- No claim should be made that all routed gameplay pathing is complete on this pass
