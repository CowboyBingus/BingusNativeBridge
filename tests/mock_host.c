/* Offline test host: plays the engine's part with real Lua 5.1.4.
 *
 * It loads the plugin DLL, hands it an engine Lua table laid out like
 * bnb_abi.h, and runs tests/test_bridge.lua against the registered module.
 * It checks this project's own declarations, not the game's layout, and is
 * never distributed.
 *
 * usage: bnb_mock_host <plugin.dll> <test_bridge.lua> <expected version>
 */
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include "bnb_abi.h"

#define TABLE_SIZE 143 /* entries the engine table holds; the plugin reads 60 */

static lua_State *host;
static bnb_entry lua_table[TABLE_SIZE];
static int failures;
static int checks;

static void check(int condition, const char *what)
{
    ++checks;
    if (!condition) {
        ++failures;
        fprintf(stderr, "FAIL: %s\n", what);
    }
}

/* Engine side of module registration: _G[module][name] = f. */
static void add_module_function(const char *module, const char *name, lua_CFunction f)
{
    lua_getglobal(host, module);
    if (!lua_istable(host, -1)) {
        lua_pop(host, 1);
        lua_newtable(host);
        lua_pushvalue(host, -1);
        lua_setglobal(host, module);
    }
    lua_pushcfunction(host, f);
    lua_setfield(host, -2, name);
    lua_pop(host, 1);
}

static void remove_module_entry(const char *module, const char *name)
{
    lua_getglobal(host, module);
    if (lua_istable(host, -1)) {
        lua_pushnil(host);
        lua_setfield(host, -2, name);
    }
    lua_pop(host, 1);
}

static void fill_lua_table(void)
{
    memset(lua_table, 0, sizeof(lua_table));
    lua_table[BNB_LUA_ADD_MODULE_FUNCTION] = (bnb_entry)add_module_function;
    lua_table[BNB_LUA_REMOVE_MODULE_ENTRY] = (bnb_entry)remove_module_entry;
    lua_table[BNB_LUA_GETTOP] = (bnb_entry)lua_gettop;
    lua_table[BNB_LUA_PUSHNIL] = (bnb_entry)lua_pushnil;
    lua_table[BNB_LUA_PUSHNUMBER] = (bnb_entry)lua_pushnumber;
    lua_table[BNB_LUA_PUSHSTRING] = (bnb_entry)lua_pushstring;
    lua_table[BNB_LUA_PUSHBOOLEAN] = (bnb_entry)lua_pushboolean;
    lua_table[BNB_LUA_CREATETABLE] = (bnb_entry)lua_createtable;
    lua_table[BNB_LUA_SETFIELD] = (bnb_entry)lua_setfield;
}

static void *full_api(unsigned id)
{
    return id == BNB_LUA_API_ID ? lua_table : NULL;
}

static void *no_api(unsigned id)
{
    (void)id;
    return NULL;
}

static int module_has(const char *name)
{
    int present;
    lua_getglobal(host, "BingusBridge");
    if (!lua_istable(host, -1)) {
        lua_pop(host, 1);
        return 0;
    }
    lua_getfield(host, -1, name);
    present = !lua_isnil(host, -1);
    lua_pop(host, 2);
    return present;
}

/* Runs one phase of the Lua tests; returns its assertion count or -1. */
static int run_lua(const char *script, const char *phase, const char *version)
{
    int top = lua_gettop(host);
    int count = -1;
    if (luaL_loadfile(host, script) != 0) {
        fprintf(stderr, "FAIL: load %s: %s\n", script, lua_tostring(host, -1));
        lua_settop(host, top);
        return -1;
    }
    lua_pushstring(host, phase);
    lua_pushstring(host, version);
    if (lua_pcall(host, 2, 1, 0) != 0)
        fprintf(stderr, "FAIL: %s phase: %s\n", phase, lua_tostring(host, -1));
    else
        count = (int)lua_tonumber(host, -1);
    lua_settop(host, top);
    return count;
}

int main(int argc, char **argv)
{
    HMODULE dll;
    void *(*get_plugin_api)(unsigned);
    struct bnb_plugin_table *plugin;
    int lua_checks = 0;
    int phase_count;
    int i;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <plugin.dll> <test_bridge.lua> <version>\n", argv[0]);
        return 2;
    }
    host = luaL_newstate();
    luaL_openlibs(host);
    fill_lua_table();

    dll = LoadLibraryA(argv[1]);
    check(dll != NULL, "plugin DLL loads");
    if (!dll)
        return 1;
    get_plugin_api = (void *(*)(unsigned))GetProcAddress(dll, "get_plugin_api");
    check(get_plugin_api != NULL, "get_plugin_api is exported");
    if (!get_plugin_api)
        return 1;

    check(get_plugin_api(BNB_LUA_API_ID) == NULL, "only the plugin table is offered");
    plugin = (struct bnb_plugin_table *)get_plugin_api(BNB_PLUGIN_API_ID);
    check(plugin != NULL, "plugin table returned");
    if (!plugin)
        return 1;
    check(plugin->get_name && strcmp(plugin->get_name(), "bingus_native_bridge") == 0, "plugin name");
    check(plugin->setup_game && plugin->shutdown_game, "setup and shutdown hooks present");
    check(plugin->unused_60 == NULL, "no per-frame hook");
    for (i = 0; i < 10; ++i)
        check(plugin->unused_08[i] == NULL, "load, reload and resource hooks unused");
    for (i = 0; i < 39; ++i)
        check(plugin->unused_70[i] == NULL, "world, unit and render hooks unused");

    /* Refusals: no engine API, no Lua table, a missing entry. */
    plugin->setup_game(NULL);
    check(!module_has("version"), "no registration without an engine API getter");
    plugin->setup_game(no_api);
    check(!module_has("version"), "no registration without the Lua table");
    lua_table[BNB_LUA_SETFIELD] = NULL;
    plugin->setup_game(full_api);
    check(!module_has("version"), "no registration when an entry is missing");
    fill_lua_table();

    /* Normal lifecycle, twice, with idempotent setup and shutdown. */
    for (i = 0; i < 2; ++i) {
        plugin->setup_game(full_api);
        plugin->setup_game(full_api);
        check(module_has("version") && module_has("capabilities") && module_has("status"),
              "module registered");
        phase_count = run_lua(argv[2], "active", argv[3]);
        check(phase_count > 0, "active phase passes");
        lua_checks += phase_count > 0 ? phase_count : 0;

        plugin->shutdown_game();
        plugin->shutdown_game();
        check(!module_has("version") && !module_has("capabilities") && !module_has("status"),
              "module entries removed at shutdown");
        phase_count = run_lua(argv[2], "after_shutdown", argv[3]);
        check(phase_count > 0, "after-shutdown phase passes");
        lua_checks += phase_count > 0 ? phase_count : 0;
    }

    lua_close(host);
    check(FreeLibrary(dll) != 0, "plugin unloads");

    if (failures) {
        fprintf(stderr, "%d of %d host checks failed\n", failures, checks);
        return 1;
    }
    printf("PASS: %d host checks, %d Lua assertions\n", checks, lua_checks);
    return 0;
}
