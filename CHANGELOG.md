# v0.1.1

- The bridge supplies the engine's required `debug_draw` and `render` plugin hooks as no-ops. Without them the game rejects the plugin before `setup_game`, so v0.1.0 would not have loaded.
- No per-frame work is added: the hooks do nothing, and the plugin still registers only `version()`, `capabilities()` and `status()`.

# v0.1.0

- First release: the foundation of the bridge. It loads as a Helldivers 2 engine plugin from `bin/plugins` and adds the `BingusBridge` Lua module with `version()`, `capabilities()` and `status()`.
- It reads and writes no game memory, runs nothing when loaded, has no per-frame work, imports no other DLLs and opens no files or network connections.
- The fair-play policy (`fair-play-1`) sets what later functions may do; `status()` reports the policy version.
