# Fair-play policy (fair-play-1)

Bingus Native Bridge exists so people can make legitimate mods for Helldivers 2 that do not give a significant unfair gameplay advantage. This policy decides which functions the bridge may offer. `BingusBridge.status()` reports the policy version the installed bridge follows.

## Principles

1. **The bridge decides what is possible, not the mod.** Every function is fixed in the bridge's own code. Mods choose among documented options; they never supply memory addresses, function pointers or code. Checks inside Lua cannot enforce anything, because every mod shares one Lua state; the bridge's functions are the only boundary.
2. **One function, one documented purpose.** Each function does a single thing that can be described in a sentence, and checks every argument.
3. **Local first.** Prefer effects only the player who installed the mod sees or feels.
4. **In the open.** The source is public, builds are reproducible, the complete list of functions is published in [API.md](API.md) and returned by `BingusBridge.capabilities()`, and every release lists the functions it adds.

## Tiers

| Tier | What it covers | Rules |
| --- | --- | --- |
| 0, information | Reading state the player can already see, or that only describes the session: the current screen, the mission phase, the player's own loadout | No information the game hides from the player |
| 1, cosmetic | Local visual and interface changes: colours, layout, interface conveniences | Seen only by the player who installed the mod |
| 2, features | Named gameplay adjustments built into the bridge | Mods can only turn them on or off or pick from fixed options. Each needs a written case that it gives no significant unfair advantage, and may be limited to solo play or private lobbies the player hosts |

## Never

The bridge will not offer, at any tier:

- reading or writing memory at an address a mod supplies, calling arbitrary functions, loading DLLs or other code, or making memory executable;
- cheats such as infinite ammunition, invulnerability, damage or fire-rate multipliers, instant cooldowns, or free resources, samples or rewards;
- information the game hides from the player, such as enemies out of sight or an uncovered map;
- automating play for an advantage, such as aim assistance or scripted input;
- working around the game's anti-cheat, integrity checks or code-signing requirements, or changing the game's code;
- network access, telemetry, or file access beyond what a documented function needs.

## Adding a function

Propose it in a public issue or pull request with:

- what it does, in one sentence, and its tier;
- who sees or feels the effect (the player only, the squad, everyone);
- why it gives no significant unfair advantage, and how it behaves in multiplayer;
- the exact arguments and their allowed values.

It is reviewed against this policy before it is merged, added to [API.md](API.md) and `capabilities()`, and listed in the next release notes. A function later found to break this policy is removed in the next release.

## Changes to this policy

The policy is versioned. A change gets a new version name, and the bridge reports the version it follows in `status().policy`.
