# Code signing policy

Helldivers 2 only loads plugins with a valid code signature. Signed releases of Bingus Native Bridge are planned through SignPath Foundation's free code signing for open-source projects; this policy describes how they are made. Until signing is in place, releases are unsigned and do not load in the game.

## What is signed

- Only `bingus_native_bridge_pluginw64_release.dll`, built from this repository by the [GitHub Actions workflow](.github/workflows/build.yml) on GitHub-hosted runners, from a tagged commit.
- No third-party or upstream binaries are signed or shipped. The offline test host and the Lua copy it builds are used in CI only and never distributed.
- The build is reproducible: anyone can rebuild a tagged commit and compare the SHA-256 published with the release.

## Team roles

| Role | Members |
| --- | --- |
| Authors (may commit without further review) | [CowboyBingus](https://github.com/CowboyBingus) |
| Reviewers (review every contribution from people without commit access) | [CowboyBingus](https://github.com/CowboyBingus) |
| Approvers (approve every signing request) | [CowboyBingus](https://github.com/CowboyBingus) |

Every team member uses multi-factor authentication for GitHub and for SignPath.

## Release process

1. Changes reach `main` only through pull requests. Continuous integration builds the plugin, runs the tests and the DLL audit, and checks that the build is reproducible.
2. A version tag builds the release artifact in CI.
3. An approver reviews every change since the previous release, checks new functions against the [fair-play policy](docs/FAIR_PLAY.md), and only then approves the signing request.
4. The signed DLL and its SHA-256 are published with the GitHub release, together with release notes that list every function added or changed.

## Privacy

This program will not transfer any information to other networked systems unless specifically requested by the user or the person installing or operating it. It makes no network connections, writes no files and collects no data.
