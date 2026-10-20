> Development release. Helldivers 2 only loads plugins that carry a valid code signature, so unsigned builds of this bridge do not load in the game. Signed releases are planned through a free code-signing program for open-source projects; see the [code signing policy](CODE_SIGNING_POLICY.md).

# Bingus Native Bridge

A small engine plugin that gives Helldivers 2 Lua mods a limited, published set of native functions through one Lua module, `BingusBridge`.

Helldivers 2 now runs plain Lua 5.1 without LuaJIT's FFI, so Lua mods can no longer reach the game's native functions on their own. The bridge brings back what legitimate mods need, and nothing more: every function it offers follows the [fair-play policy](docs/FAIR_PLAY.md). That means no function that gives a significant unfair gameplay advantage, no access to memory at addresses a mod chooses, and no way to run other code.

Not affiliated with or endorsed by Arrowhead Game Studios or Sony Interactive Entertainment.

## Version 0.1

Version 0.1 is the foundation. It loads as an engine plugin and adds three information functions:

| Function | Returns |
| --- | --- |
| `BingusBridge.version()` | the version string and the API level |
| `BingusBridge.capabilities()` | every function the bridge offers, with its tier |
| `BingusBridge.status()` | version, API level, policy version, function count and whether any function touches game memory (none do) |

Full reference: [Lua API](docs/API.md). Later versions add functions one by one, each reviewed against the policy and listed in the release notes.

## What the plugin does and does not do

- It is loaded by the game's own plugin system from `bin/plugins`, like the game's stock plugins.
- Nothing runs when the DLL is loaded: it has no entry point and imports no other DLLs. Its only export is the engine's plugin handshake.
- When the game sets up Lua, it registers the `BingusBridge` functions; when the game shuts down, it removes them.
- It has no per-frame work. Its functions run only when a mod calls them.
- It reads and writes no game memory, opens no files and makes no network connections.
- It does not modify the game's files or code and does not work around the game's anti-cheat or its signature checks.

## Install

1. Close Helldivers 2.
2. Copy `bingus_native_bridge_pluginw64_release.dll` into `Helldivers 2\bin\plugins` (in Steam: Manage, Browse local files).
3. Start the game.

## Uninstall

Close the game and delete `bingus_native_bridge_pluginw64_release.dll` from `Helldivers 2\bin\plugins`. Nothing else is installed or changed.

## Building from source

Requirements: Windows, Visual Studio with the C++ desktop workload (MSVC for x64), CMake 3.21 or newer, Python 3 for the audit test.

```
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The plugin is `build/Release/bingus_native_bridge_pluginw64_release.dll`. The build is reproducible: the same source gives the same file, byte for byte. The tests download the official Lua 5.1.4 source (checked against its SHA-256) for an offline test host; to build without a download, pass `-DBNB_LUA_URL=<path to lua-5.1.4.tar.gz>`. The test host is never distributed.

Every push and pull request runs the same build, the tests, an audit of the DLL (`tools/pe_audit.py`) and a second build to confirm it is reproducible ([workflow](.github/workflows/build.yml)).

## Code signing policy

See [CODE_SIGNING_POLICY.md](CODE_SIGNING_POLICY.md).

## Reporting security problems or misuse

See [SECURITY.md](SECURITY.md).

## License

[BSD Zero Clause License (0BSD)](LICENSE).
