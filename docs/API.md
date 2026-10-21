# BingusBridge Lua API (API level 1)

The bridge registers its functions in the `BingusBridge` module when the game sets up Lua. Everything listed here is everything that exists; see the [fair-play policy](FAIR_PLAY.md) for what may be added and how.

## Conventions

- Functions accept only the documented arguments. A call with anything else returns `nil` and a message; the bridge never raises a Lua error.
- After the game shuts the plugin down, the module's entries are removed. References a mod kept still exist but return nothing.
- `version()` returns an API level. It rises only when an existing function changes incompatibly; new functions do not change it, so test for a function with `capabilities()` instead.

## Finding the bridge from a mod

```lua
local function find_bridge()
    local bridge = rawget(_G, "BingusBridge")
    if bridge == nil then
        local engine = rawget(_G, "stingray")
        bridge = engine and engine.BingusBridge
    end
    if type(bridge) == "table" and bridge.version then
        return bridge
    end
end

local bridge = find_bridge()
if bridge then
    local version, api_level = bridge.version()
end
```

## Functions

### `BingusBridge.version()`

Tier 0. No arguments.

Returns the bridge version as a string (`"0.1.1"`) and the API level as a number (`1`).

### `BingusBridge.capabilities()`

Tier 0. No arguments.

Returns a table that maps the name of every function the bridge offers to its tier:

```lua
{ version = 0, capabilities = 0, status = 0 }
```

### `BingusBridge.status()`

Tier 0. No arguments.

Returns a table:

| Field | Type | Meaning |
| --- | --- | --- |
| `version` | string | bridge version |
| `api_level` | number | API level |
| `policy` | string | the fair-play policy version the bridge follows (`"fair-play-1"`) |
| `capability_count` | number | how many functions the bridge offers |
| `game_memory` | boolean | whether any function reads or writes game memory (`false` in this version) |
