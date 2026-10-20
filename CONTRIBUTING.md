# Contributing

This repository is licensed under the BSD Zero Clause License (0BSD); see [LICENSE](LICENSE). Contributions are accepted under the same license.

## Build and test

```
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

To build without downloading the Lua source for the test host, pass `-DBNB_LUA_URL=<path to lua-5.1.4.tar.gz>`; its SHA-256 is still checked.

## Rules for changes

- Every change goes through a pull request, and CI must pass.
- A new Lua function must follow the [fair-play policy](docs/FAIR_PLAY.md). Say in the pull request what it does, its tier, who sees the effect, and why it gives no significant unfair advantage. Add it to [docs/API.md](docs/API.md) and to the tests.
- Keep the plugin free of the C runtime, an entry point and imports, with `get_plugin_api` as its only export. `tools/pe_audit.py` checks this in CI.
- Refusals return `nil` and a message; the plugin never raises Lua errors.
- No game files, data copied from the game, or third-party SDK files in the repository. The engine interface in `src/bnb_abi.h` is this project's own declaration of the slots the plugin uses.
- Add a line to `CHANGELOG.md` and `docs/RELEASE_NOTES.md` for every change a user would notice.
