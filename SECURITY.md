# Security and misuse

Please report privately through **Report a vulnerability** on this repository's Security tab, so a problem can be fixed before it is public.

Report anything that lets the bridge be used beyond its [published functions](docs/API.md) or its [fair-play policy](docs/FAIR_PLAY.md), in particular:

- reading or writing memory, calling functions or running code through the bridge in a way the API does not document;
- a function that gives a significant unfair gameplay advantage;
- a crash or memory-safety bug in the plugin;
- a released DLL that does not match a rebuild of its tagged source.

A function found to break the policy is removed in the next release, and a fix for a security problem is released as soon as it is ready. Only the latest release is supported.
