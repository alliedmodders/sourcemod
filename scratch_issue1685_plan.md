# Fix Plan: Dangling `IGameEvent` pointer when a plugin blocks an event

**Issue:** [alliedmodders/sourcemod#1685](https://github.com/alliedmodders/sourcemod/issues/1685)
**Area:** `core/EventManager.cpp`, `core/EventManager.h` (per-SDK core)

## Root cause (confirmed)

`EventManager::OnFireEvent` (`core/EventManager.cpp:436-440`) frees the event
**inside the pre-hook** and then returns `MRES_SUPERCEDE`:

```cpp
if (res >= Pl_Handled)
{
    gameevents->FreeEvent(pEvent);
    RETURN_META_VALUE(MRES_SUPERCEDE, false);
}
```

`MRES_SUPERCEDE` only suppresses the engine's original `FireEvent` — it does **not**
stop lower-priority hooks in the same SourceHook chain. Those hooks (other
extensions / Metamod plugins) then run with a freed `pEvent` → dangling pointer / crash.

The reporter's suggested fix ("just remove the `FreeEvent()` calls") is wrong: in the
superseded path the engine never runs, so nothing else frees the event. Removing the
calls leaks one `IGameEvent` every time a plugin blocks an event.

## Key facts that make the real fix safe

- **SourceHook always runs the post-hook even after a supersede.** Proof from the
  existing code: the pre-hook pushes to `m_EventStack` unconditionally (`:407`/`:444`)
  and the post-hook pops unconditionally (`:513`). If post-hooks were skipped on
  supersede, the stack would desync on every blocked event and SM would have been
  crashing for years. It doesn't — so the post-hook is a reliable place to defer work.
- **The post-hook never touches the real `pEvent`.** In `OnFireEvent_Post`, the
  non-copy path pushes `BAD_HANDLE` (`:484`) and the copy path uses the *duplicated*
  event from `m_EventCopies` (`:478`), not `pEvent`. So `pEvent` is free to be freed there.
- **Freeing in the post-hook matches engine timing.** Normally the engine's `FireEvent`
  frees the event between the pre and post phases, so post-hooks already can't rely on
  `pEvent`. Deferring the free to SM's post-hook reproduces that contract instead of
  violating it — no regression.
- The handle freed at `:428` does **not** free `pEvent` (the stack-local `EventInfo`
  has `pOwner == NULL`, so `OnHandleDestroy` is a no-op for it). The explicit
  `FreeEvent` at `:438` is the only thing releasing the event, so moving that single
  call is the entire behavioral change.

## Changes

### 1. `core/EventManager.h`

Add a parallel stack to carry the deferred free across the pre→post transition,
pushed/popped in lockstep with `m_EventStack`:

```cpp
CStack<IGameEvent *> m_EventDeferredFree;  // pEvent to free in post-hook, or NULL
```

A pointer stack (rather than a bool stack) means the post-hook frees the exact
pointer captured at supersede time — robust against nested/reentrant `FireEvent`
calls, which the stack design already anticipates.

### 2. `core/EventManager.cpp` — `OnFireEvent` (pre-hook)

Push to `m_EventDeferredFree` in lockstep with `m_EventStack` in **both** branches
(hooked and unhooked) so the two stacks stay balanced:

- unhooked branch (`:444`): push `NULL`.
- hooked branch: push `NULL` by default.

Replace the blocked path (`:436-440`):

```cpp
if (res >= Pl_Handled)
{
    // Defer free to the post-hook so lower-priority hooks in the
    // chain still see a valid event (issue #1685).
    m_EventDeferredFree.top() = pEvent;   // overwrite the NULL pushed above
    RETURN_META_VALUE(MRES_SUPERCEDE, false);
}
```

(Exact mechanism — top-overwrite vs. deferring the push — finalized during
implementation; intent is one push per fire, balanced with `m_EventStack`.)

### 3. `core/EventManager.cpp` — `OnFireEvent_Post`

After the existing post-forward work, alongside the `m_EventStack.pop()` (`:513`),
pop the parallel stack and free if set:

```cpp
IGameEvent *pDeferred = m_EventDeferredFree.front();
m_EventDeferredFree.pop();
if (pDeferred)
    gameevents->FreeEvent(pDeferred);
```

Guard stack balance for the early `!pEvent` returns: the post-hook `!pEvent` return
(`:462`) and the pre-hook `!pEvent` return (`:394`) both return **before** any push,
so neither must pop. Verify both paths stay symmetric — this is the one bit of
bookkeeping to get exactly right.

## Test

Add a mock-testsuite plugin under `plugins/testsuite/mock/` (register in
`.github/workflows/mocktest.yml`): hook a common event in `EventHookMode_Pre`, return
`Plugin_Handled`, and confirm no crash / correct suppression.

To actually exercise the dangling-pointer path we need a *second* consumer of
`IGameEventManager2::FireEvent` at lower priority; since that's hard to stage from
pure SourcePawn, document the manual repro (two extensions, one blocking) in the PR
and rely on the mock test for the no-regression guarantee.

## Build & verify

- Rebuild core against mock: `--sdks=mock --targets=x86_64`, assemble gamedir, run the
  new test via `srcds ... -run` and confirm no `FAIL`.
- This is per-SDK core code (not logic), so it compiles once per SDK, but the change is
  engine-agnostic; a mock build is sufficient to validate.

## Scope / risk notes

- Contained to `EventManager.{h,cpp}`; no API/`.inc` surface change, no gamedata.
- The bug is niche (needs a second lower-priority `FireEvent` hooker), which is why
  it's sat open since 2022 — worth calling out in the PR so reviewers understand the trigger.
- Per `CLAUDE.md`, nontrivial changes should be discussed with the dev team first — this
  is PR-with-issue-reference material, not a drive-by.
