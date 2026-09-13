# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

SourceMod is a Metamod:Source plugin for Source engine game servers, providing server administration and a scripting environment (SourcePawn). C++17, built with AMBuild 2.2+. Targets Windows and Linux, x86 and x86_64, against multiple HL2SDK engine branches simultaneously.

## Build

Dependencies (HL2SDKs, Metamod:Source, MySQL, AMBuild) are fetched by `tools/checkout-deps.ps1` / `tools/checkout-deps.sh`, run from a sibling directory of the repo checkout (it expects a `sourcemod` folder next to it). Example: `tools/checkout-deps.sh -s tf2,css` to limit SDKs.

Configure and build (out-of-source):

```
mkdir build && cd build
python ../configure.py --enable-optimize --sdks=present --targets=x86,x86_64
ambuild
```

- To rebuild after edits, just run `ambuild` from (or pointed at) the existing build dir — e.g. `ambuild build-win`. This repo already has configured output dirs: `build-win`, `build-win32`, `build-lin`.
- Key configure flags: `--sdks=` (comma list, `present`, or `all`), `--targets=x86,x86_64`, `--no-mysql`, `--enable-debug`, `--hl2sdk-root=`, `--mms-path=`, `--scripting-only` (build only spcomp + scripting package).
- The packaged layout lands in `<builddir>/package` (an `addons/sourcemod` tree).
- GCC builds use `-Wall -Werror`; MSVC 2017 15.7+ / GCC 9+ / Clang 5+ required.

## Tests

There is no unit test runner in this repo; functional tests are SourcePawn plugins in `plugins/testsuite/`, with the CI-run subset in `plugins/testsuite/mock/` (see `.github/workflows/mocktest.yml`). They run against [hl2sdk-mock](https://github.com/alliedmodders/hl2sdk-mock), a fake Source engine:

1. Build metamod-source and SourceMod with `--sdks=mock --targets=x86_64`, build hl2sdk-mock, then assemble a gamedir with hl2sdk-mock's `build_gamedir.sh` from both package dirs.
2. Compile a test: `spcomp64 -i <gamedir>/addons/sourcemod/scripting/include -o test.smx -E plugins/testsuite/mock/<test>.sp`
3. Run one test: `./srcds -game_dir <gamedir> +map de_thunder -command "sm plugins load optional/<test>" -run -run-ticks 20` — output containing `FAIL` means failure.

## Architecture

Runtime loading chain: **loader → core (per-engine) → logic (engine-agnostic)**.

- `loader/` — builds `sourcemod_mm`, the Metamod:Source plugin stub. At runtime it detects the engine branch and loads the matching core binary (`sourcemod.2.<sdk>`, e.g. `sourcemod.2.tf2`).
- `core/` — engine-dependent core, compiled **once per SDK** (each `sourcemod.2.<sdk>` binary): console command/convar hooks, events, menus, players, timers. `core/logic_bridge.cpp` implements `CoreProvider`.
- `core/logic/` — `sourcemod.logic`, compiled once, engine-agnostic: plugin system, handles, natives, admin cache, extensions manager, database support, translator.
- `bridge/` — the interface boundary between core and logic (`CoreProvider`/`LogicProvider` structs in `bridge/include/`). Anything logic needs from the engine goes through here.
- `sourcepawn/` — **git submodule**: the SourcePawn VM/JIT and compiler (`spcomp`). Don't edit here for SourceMod changes; it's updated by bumping the submodule.
- `public/amtl`, `public/safetyhook`, `hl2sdk-manifests/` — other submodules (AlliedModders template library, detour library, SDK build manifests consumed by `AMBuildScript`).
- `extensions/` — bundled C++ extensions, each with its own `AMBuilder` (registered in the root `AMBuildScript`). Engine-dependent ones (sdktools, sdkhooks, cstrike, tf2, dhooks) build per-SDK; others (mysql, regex, geoip, clientprefs…) build once.
- `plugins/` — bundled SourcePawn plugins. `plugins/include/` is the **public scripting API** (`.inc` files) — changes here change the API surface for every plugin author and are documented at sm.alliedmods.net/new-api.
- `gamedata/` — per-game offsets/signatures/patch info (KeyValues text files), often updated for game patches without code changes.
- `configs/`, `translations/` — shipped default configs and localization phrase files. Translations have their own sanity-check CI workflows.
- `versionlib/` + `product.version` — auto-versioning; generated headers end up in `<builddir>/includes` (disable with `--disable-auto-versioning`).

Extensions interface with core via the public SDK headers in `public/` (e.g. `IExtensionSys.h`, `IPluginSys.h`), the same headers third-party extension authors use (`public/sample_ext` is the template).

## Contributing notes

- Nontrivial changes are expected to be discussed in an issue or with the dev team before a PR (see `.github/CONTRIBUTING.md`); new configuration options are very rarely accepted.
- Security issues go to security@alliedmods.net, not the public tracker.
- PR branches target `master` or the active `X.Y-dev` branch.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
