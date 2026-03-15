# Project Overview

AscentEmu 2.4.3 is a continuation of the final revision of OpenAscent, preserved and extended as a free, educational emulator project.
The goal of this project is to provide a stable reference implementation for learning, experimentation, and community-driven improvement.

Contributions, bug reports, and fixes are welcome.

# Key Features
- Based on the final OpenAscent 2.4.3 codebase
- Focused on stability, correctness, and maintainability
- Intended for learning, experimentation, and emulator development
- Open to community contributions and issue submissions

# Database
- Recommended SQL Server: MariaDB
- Recommended Database Editor: HeidiSQL

# Core Required Items

Map Extractor Location: \Alpha\Core\extras\extracted

The same directory includes:
- Pre-extracted 2.4.3 DBC files
- A DBC Extractor if you prefer to generate fresh DBCs


# Build Information
- Supported Platform: Windows (currently)
- Build Configuration: Release x64
- Debug builds are not recommended for normal operation.
- First-time Windows builds should stage repo-local dependencies before opening the solution:
  `.\Alpha\Core\win\bootstrap-deps.ps1 -Platform x64 -Source VendorFallback -Force`
- The Windows solution builds `pcre` and `zlib` from source in-repo and stages MySQL/OpenSSL under `Alpha/Core/.deps`.
- Supported server topology: single-realm deployments using `ascent-world` and `ascent-logonserver`
- Legacy `realmserver`/clustering support has been retired from the active build.
- `LUAScripting` is retired and is not part of the active build.
- Support for additional platforms is planned for future development.

Windows build documentation:
- `Alpha/Core/win/BUILD_WINDOWS.md`
- `Alpha/Core/win/DEPENDENCIES.md`


# Project Status

This project is actively maintained as a learning and preservation effort.
While not intended to be a production-grade emulator, it aims to remain clean, understandable, and extensible for developers exploring WoW emulator internals.
