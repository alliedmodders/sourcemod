# Repository Guidelines

## Project Structure & Module Organization

SourceMod is a C++17 Metamod:Source plugin with a SourcePawn scripting layer. The runtime path is `loader/` (Metamod entry point) to the per-SDK `core/` to engine-independent `core/logic/`; interfaces between the latter two live in `bridge/include/`. Bundled C++ extensions are in `extensions/`, while bundled SourcePawn plugins and public `.inc` APIs are in `plugins/` and `plugins/include/`. Keep game offsets and signatures in `gamedata/`, defaults in `configs/`, and phrase files in `translations/`. `sourcepawn/`, `public/amtl`, `public/safetyhook`, and `hl2sdk-manifests/` are submodules: do not make local product changes there.

## Build, Test, and Development Commands

Fetch dependencies from a sibling checkout using `tools/checkout-deps.ps1` (Windows) or `tools/checkout-deps.sh`; these scripts expect this repository to be named `sourcemod`.

```text
mkdir build; cd build
python ../configure.py --enable-optimize --sdks=present --targets=x86,x86_64
ambuild
```

This produces the distributable tree in `build/package/`. Rebuild an existing output directory with `ambuild build-win` (or the relevant build directory). Use `--enable-debug`, `--no-mysql`, or `--scripting-only` when appropriate. Verify translation-only changes with `python tools/language_check/sanity_check.py`.

## Coding Style & Naming Conventions

Follow the surrounding file exactly. C++ uses four-space indentation, brace-on-next-line function definitions, and established `CamelCase` types/functions with `m_` member prefixes. SourcePawn uses four spaces, `#pragma semicolon 1`, `#pragma newdecls required`, and `PascalCase` callback/function names; name test plugins `test_<feature>.sp`. Do not add a formatter configuration or reformat unrelated code. Public API changes in `plugins/include/` require especially careful compatibility review.

## Testing Guidelines

There is no standalone unit-test command. Add functional SourcePawn coverage under `plugins/testsuite/`; CI mock tests belong in `plugins/testsuite/mock/`. The mock workflow builds with `--sdks=mock --targets=x86_64`, compiles each test with `spcomp64`, and fails on `FAIL` output. Run a targeted built plugin against a suitable Source server or mirror `.github/workflows/mocktest.yml` for full mock coverage.

## Commits & Pull Requests

Use short imperative subjects, optionally scoped, such as `gamedata: update PVKII offsets` or `Fix build`; include an issue reference when relevant. Keep commits focused. Discuss nontrivial changes before opening a PR—new configuration options are rarely accepted. Target `master` or the active `X.Y-dev` branch, describe behavior and testing, link the issue, and include a minimal reproduction for bug fixes. Report security issues privately to `security@alliedmods.net`.
