/* Engine plugin interface, declared by this project for interoperability.
 *
 * Only the entries this plugin touches are named. The engine fixes the order
 * of both tables; the offsets are checked at compile time so a layout mistake
 * fails the build instead of the game.
 */
#ifndef BNB_ABI_H
#define BNB_ABI_H

#include <stddef.h>

typedef struct lua_State lua_State;
typedef int (*bnb_cfunction)(lua_State *L);
typedef void *(*bnb_get_api)(unsigned api_id);
typedef void (*bnb_entry)(void); /* untyped engine table entry */

#define BNB_PLUGIN_API_ID 0u
#define BNB_LUA_API_ID 1u

/* Table the plugin returns from get_plugin_api(BNB_PLUGIN_API_ID). */
struct bnb_plugin_table {
    const char *(*get_name)(void);   /* 0x00 */
    void *unused_08[10];             /* 0x08 load, reload, compiler and resource hooks */
    void (*setup_game)(bnb_get_api); /* 0x58 Lua accepts module functions from here on */
    void *unused_60;                 /* 0x60 per-frame hook: deliberately left empty */
    void (*shutdown_game)(void);     /* 0x68 */
    void *unused_70[39];             /* 0x70 world, unit and render hooks, reserved space */
};

/* Positions of the engine Lua table entries this plugin reads. */
enum bnb_lua_slot {
    BNB_LUA_ADD_MODULE_FUNCTION = 0,
    BNB_LUA_REMOVE_MODULE_ENTRY = 9,
    BNB_LUA_GETTOP = 13,
    BNB_LUA_PUSHNIL = 39,
    BNB_LUA_PUSHNUMBER = 40,
    BNB_LUA_PUSHSTRING = 43,
    BNB_LUA_PUSHBOOLEAN = 47,
    BNB_LUA_CREATETABLE = 54,
    BNB_LUA_SETFIELD = 59,
    BNB_LUA_SLOTS_READ = 60 /* entries 0..59 at most; nothing beyond */
};

#define BNB_ABI_ASSERT(condition, name) typedef char bnb_abi_##name[(condition) ? 1 : -1]
BNB_ABI_ASSERT(sizeof(void *) == 8, x64_only);
BNB_ABI_ASSERT(offsetof(struct bnb_plugin_table, setup_game) == 0x58, setup_game_at_0x58);
BNB_ABI_ASSERT(offsetof(struct bnb_plugin_table, unused_60) == 0x60, frame_hook_at_0x60);
BNB_ABI_ASSERT(offsetof(struct bnb_plugin_table, shutdown_game) == 0x68, shutdown_game_at_0x68);
BNB_ABI_ASSERT(sizeof(struct bnb_plugin_table) == 0x1a8, plugin_table_size);

#endif
