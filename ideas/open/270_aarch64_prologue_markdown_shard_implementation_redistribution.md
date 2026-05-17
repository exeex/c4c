# AArch64 `prologue.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/prologue.md` back into real compiled
owners named `prologue.cpp` and `prologue.hpp`, then delete the markdown shard.

The prologue owner should reflect current prepared frame/call contracts and
explicit deferred behavior, not a direct import of the older reference frame
pipeline.

## Why This Exists

Function entry, frame setup, parameter homes, callee saves, and epilogue
behavior are easy to scatter across calls, returns, memory, and module compile
code. The markdown shard still describes a historical owner, so a live compiled
owner is needed to keep frame-entry responsibilities reviewable.

## In Scope

- Create `src/backend/mir/aarch64/codegen/prologue.cpp` and
  `src/backend/mir/aarch64/codegen/prologue.hpp`.
- Move any current AArch64 prologue/epilogue/frame-entry construction or
  explicit deferred hooks into the prologue owner.
- Align the owner with
  `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` and prepared
  frame/call facts.
- Keep calls and returns owners focused on their narrower behavior while
  routing frame-entry work through prologue where appropriate.
- Delete `prologue.md` once reconciled.

## Out of Scope

- Rebuilding full frame layout, register allocation, or variadic save-area
  machinery from the reference backend.
- Changing ABI semantics, callee-save behavior, stack layout, or emitted
  prologue/epilogue sequences except for behavior-preserving movement.
- Inventing missing prepared frame facts.
- Redistributing other markdown shards.

## Completion Criteria

- `prologue.cpp` and `prologue.hpp` exist and own the current prologue/frame
  entry boundary or explicit deferred handling.
- `prologue.md` is deleted.
- Frame-entry behavior is not hidden in broad or unrelated owners.
- Focused AArch64 backend proof preserves existing ABI-visible behavior.

## Reviewer Reject Signals

Reject the route or slice if it:

- changes stack layout, callee-save, parameter, return, or frame semantics
  while claiming ownership-only redistribution;
- ports historical prologue code without current prepared-frame authority;
- hides frame-entry behavior in calls, returns, memory, or backend driver code;
- leaves `prologue.md` in place after claiming reconciliation;
- downgrades ABI or backend tests to pass the refactor.
