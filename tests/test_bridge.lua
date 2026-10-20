-- Offline tests for the BingusBridge module, run by tests/mock_host.c.
-- Arguments: phase ("active" or "after_shutdown"), expected version.
-- Returns the number of assertions that ran; any failure raises an error.

local phase, expected_version = ...
local count = 0

local function check(condition, what)
    count = count + 1
    if not condition then
        error(what, 2)
    end
end

local EXPECTED = { version = 0, capabilities = 0, status = 0 }

local function sorted_keys(t)
    local keys = {}
    for key in pairs(t) do
        keys[#keys + 1] = tostring(key)
    end
    table.sort(keys)
    return table.concat(keys, ",")
end

local function active()
    local bridge = BingusBridge
    check(type(bridge) == "table", "BingusBridge module exists")
    check(sorted_keys(bridge) == "capabilities,status,version",
        "module holds exactly the listed functions, got " .. sorted_keys(bridge))

    local version, api_level = bridge.version()
    check(version == expected_version, "version string")
    check(api_level == 1, "API level")

    local caps = bridge.capabilities()
    check(type(caps) == "table", "capabilities returns a table")
    check(sorted_keys(caps) == sorted_keys(EXPECTED), "capability names")
    for name, tier in pairs(EXPECTED) do
        check(caps[name] == tier, "tier of " .. name)
    end

    local status = bridge.status()
    check(status.version == expected_version, "status version")
    check(status.api_level == 1, "status API level")
    check(status.policy == "fair-play-1", "status policy")
    check(status.capability_count == 3, "status capability count")
    check(status.game_memory == false, "status reports no game-memory access")

    -- Refusals: arguments are rejected with nil and a message, never an error.
    for _, name in ipairs({ "version", "capabilities", "status" }) do
        local result, message = bridge[name](1)
        check(result == nil, name .. " refuses arguments")
        check(message == "BingusBridge." .. name .. " takes no arguments", name .. " refusal message")
    end

    -- Nothing beyond the list exists, whatever a mod tries to call.
    for _, name in ipairs({ "read", "write", "call", "load", "address", "memory", "exec" }) do
        check(bridge[name] == nil, "no " .. name .. " function")
    end

    -- Repeated calls stay stable.
    for _ = 1, 10000 do
        local v = bridge.version()
        if v ~= expected_version then
            check(false, "repeated version call")
        end
        bridge.capabilities()
        bridge.status()
    end
    count = count + 1

    saved_bridge_functions = { bridge.version, bridge.capabilities, bridge.status }
end

local function after_shutdown()
    local bridge = BingusBridge
    check(bridge == nil or sorted_keys(bridge) == "", "module entries removed")
    check(type(saved_bridge_functions) == "table", "references kept from the active phase")
    for index, f in ipairs(saved_bridge_functions) do
        check(select("#", f()) == 0, "kept reference " .. index .. " returns nothing after shutdown")
    end
    saved_bridge_functions = nil
end

if phase == "active" then
    active()
elseif phase == "after_shutdown" then
    after_shutdown()
else
    error("unknown phase " .. tostring(phase))
end
return count
