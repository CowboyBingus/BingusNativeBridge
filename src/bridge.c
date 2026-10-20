/* Bingus Native Bridge: engine plugin that adds the BingusBridge Lua module.
 *
 * Version 0.1 is the foundation. It registers three information functions
 * and the capability list later versions extend under docs/FAIR_PLAY.md.
 * It reads and writes no game memory, makes no system calls, loads no code,
 * has no per-frame hook and runs nothing when the DLL is loaded.
 */
#include "bnb_abi.h"
#include "bnb_version.h"

/* Built without the C runtime; the compiler expects this symbol once
 * floating-point values (Lua numbers) are used. */
int _fltused = 0;

/* Typed copies of the engine Lua entries, bound once in setup_game. */
struct bnb_lua {
    void (*add_module_function)(const char *module, const char *name, bnb_cfunction f);
    void (*remove_module_entry)(const char *module, const char *name);
    int (*gettop)(lua_State *L);
    void (*pushnil)(lua_State *L);
    void (*pushnumber)(lua_State *L, double n);
    void (*pushstring)(lua_State *L, const char *s);
    void (*pushboolean)(lua_State *L, int b);
    void (*createtable)(lua_State *L, int narr, int nrec);
    void (*setfield)(lua_State *L, int index, const char *key);
};

/* Tiers from docs/FAIR_PLAY.md. Version 0.1 only has information functions. */
enum bnb_tier {
    BNB_TIER_INFO = 0,     /* safe reads */
    BNB_TIER_COSMETIC = 1, /* local, visual only */
    BNB_TIER_FEATURE = 2   /* named gameplay features, on or off */
};

struct bnb_capability {
    const char *name;
    enum bnb_tier tier;
    bnb_cfunction function;
};

static int lua_version(lua_State *L);
static int lua_capabilities(lua_State *L);
static int lua_status(lua_State *L);

/* The whole Lua surface. Anything not listed here does not exist. */
static const struct bnb_capability capabilities[] = {
    {"version", BNB_TIER_INFO, lua_version},
    {"capabilities", BNB_TIER_INFO, lua_capabilities},
    {"status", BNB_TIER_INFO, lua_status},
};
#define CAPABILITY_COUNT ((int)(sizeof(capabilities) / sizeof(capabilities[0])))

static struct bnb_lua lua;
static int active;

/* Refusals return nil and a message rather than raising a Lua error, so no
 * error unwinds through this plugin whatever mechanism the engine uses. */
static int refuse(lua_State *L, const char *message)
{
    lua.pushnil(L);
    lua.pushstring(L, message);
    return 2;
}

static void set_number(lua_State *L, const char *key, double value)
{
    lua.pushnumber(L, value);
    lua.setfield(L, -2, key);
}

static void set_string(lua_State *L, const char *key, const char *value)
{
    lua.pushstring(L, value);
    lua.setfield(L, -2, key);
}

static void set_boolean(lua_State *L, const char *key, int value)
{
    lua.pushboolean(L, value);
    lua.setfield(L, -2, key);
}

/* version() -> version string, API level */
static int lua_version(lua_State *L)
{
    if (!active)
        return 0;
    if (lua.gettop(L) != 0)
        return refuse(L, BNB_MODULE ".version takes no arguments");
    lua.pushstring(L, BNB_VERSION_STRING);
    lua.pushnumber(L, BNB_API_LEVEL);
    return 2;
}

/* capabilities() -> {name = tier, ...} */
static int lua_capabilities(lua_State *L)
{
    int i;
    if (!active)
        return 0;
    if (lua.gettop(L) != 0)
        return refuse(L, BNB_MODULE ".capabilities takes no arguments");
    lua.createtable(L, 0, CAPABILITY_COUNT);
    for (i = 0; i < CAPABILITY_COUNT; ++i)
        set_number(L, capabilities[i].name, capabilities[i].tier);
    return 1;
}

/* status() -> {version, api_level, policy, capability_count, game_memory} */
static int lua_status(lua_State *L)
{
    if (!active)
        return 0;
    if (lua.gettop(L) != 0)
        return refuse(L, BNB_MODULE ".status takes no arguments");
    lua.createtable(L, 0, 5);
    set_string(L, "version", BNB_VERSION_STRING);
    set_number(L, "api_level", BNB_API_LEVEL);
    set_string(L, "policy", BNB_POLICY);
    set_number(L, "capability_count", CAPABILITY_COUNT);
    set_boolean(L, "game_memory", 0);
    return 1;
}

static int has_slots(const bnb_entry *slots)
{
    static const int needed[] = {
        BNB_LUA_ADD_MODULE_FUNCTION, BNB_LUA_REMOVE_MODULE_ENTRY, BNB_LUA_GETTOP,
        BNB_LUA_PUSHNIL, BNB_LUA_PUSHNUMBER, BNB_LUA_PUSHSTRING,
        BNB_LUA_PUSHBOOLEAN, BNB_LUA_CREATETABLE, BNB_LUA_SETFIELD,
    };
    int i;
    for (i = 0; i < (int)(sizeof(needed) / sizeof(needed[0])); ++i) {
        if (!slots[needed[i]])
            return 0;
    }
    return 1;
}

/* Copies the needed entries; refuses a missing table or entry. */
static int bind_lua(const bnb_entry *slots)
{
    if (!slots || !has_slots(slots))
        return 0;
    lua.add_module_function = (void (*)(const char *, const char *, bnb_cfunction))
        slots[BNB_LUA_ADD_MODULE_FUNCTION];
    lua.remove_module_entry = (void (*)(const char *, const char *))
        slots[BNB_LUA_REMOVE_MODULE_ENTRY];
    lua.gettop = (int (*)(lua_State *))slots[BNB_LUA_GETTOP];
    lua.pushnil = (void (*)(lua_State *))slots[BNB_LUA_PUSHNIL];
    lua.pushnumber = (void (*)(lua_State *, double))slots[BNB_LUA_PUSHNUMBER];
    lua.pushstring = (void (*)(lua_State *, const char *))slots[BNB_LUA_PUSHSTRING];
    lua.pushboolean = (void (*)(lua_State *, int))slots[BNB_LUA_PUSHBOOLEAN];
    lua.createtable = (void (*)(lua_State *, int, int))slots[BNB_LUA_CREATETABLE];
    lua.setfield = (void (*)(lua_State *, int, const char *))slots[BNB_LUA_SETFIELD];
    return 1;
}

static const char *get_name(void)
{
    return BNB_PLUGIN_NAME;
}

static void setup_game(bnb_get_api get_engine_api)
{
    int i;
    if (active || !get_engine_api)
        return;
    if (!bind_lua((const bnb_entry *)get_engine_api(BNB_LUA_API_ID)))
        return;
    for (i = 0; i < CAPABILITY_COUNT; ++i)
        lua.add_module_function(BNB_MODULE, capabilities[i].name, capabilities[i].function);
    active = 1;
}

/* Removes the module entries. References Lua kept stay callable while the
 * DLL is loaded but return nothing. */
static void shutdown_game(void)
{
    int i;
    if (!active)
        return;
    active = 0;
    for (i = 0; i < CAPABILITY_COUNT; ++i)
        lua.remove_module_entry(BNB_MODULE, capabilities[i].name);
}

static struct bnb_plugin_table plugin_table = {
    .get_name = get_name,
    .setup_game = setup_game,
    .shutdown_game = shutdown_game,
};

/* The only export. The engine calls it after loading the plugin. */
__declspec(dllexport) void *get_plugin_api(unsigned api_id)
{
    return api_id == BNB_PLUGIN_API_ID ? &plugin_table : 0;
}
