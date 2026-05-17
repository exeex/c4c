# AArch64 `variadic.md` Shard Implementation Redistribution

Status: Closed
Created: 2026-05-17
Closed: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/variadic.md` back into real compiled
owners named `variadic.cpp` and `variadic.hpp`, then delete the markdown shard.

The variadic owner should preserve current accepted AAPCS64 variadic call
facts and explicit deferred function-entry/`va_list` behavior. It should not
pretend that the full historical `va_start`/`va_arg` implementation exists
before the prepared facts are available.

## Why This Exists

Variadic ABI behavior cuts across calls, prologue/frame setup, memory, and
intrinsics. Leaving its shard as markdown makes future work likely to land in
the wrong owner or to overfit a single variadic test. A dedicated compiled
boundary makes the supported and deferred pieces visible.

## In Scope

- Create `src/backend/mir/aarch64/codegen/variadic.cpp` and
  `src/backend/mir/aarch64/codegen/variadic.hpp`.
- Move current AArch64 variadic call-boundary handling or explicit deferred
  hooks into the variadic owner.
- Preserve existing prepared facts such as direct extern variadic wrapper kind
  and variadic FPR argument register count.
- Keep function-entry `va_list`, register-save-area, and `va_arg` behavior
  deferred unless structured prepared facts already exist.
- Delete `variadic.md` once reconciled.

## Out of Scope

- Implementing full AAPCS64 `va_start`, `va_arg`, or `va_copy` in this
  ownership-cleanup idea.
- Inventing missing prepared variadic carriers.
- Changing call ABI, prologue, memory, or aggregate copy semantics outside
  behavior-preserving ownership movement.
- Rewriting tests or expectations to claim unsupported variadic function-entry
  support.
- Redistributing other markdown shards.

## Completion Criteria

- `variadic.cpp` and `variadic.hpp` exist and own the current variadic boundary
  or explicit deferred handling.
- `variadic.md` is deleted.
- Variadic behavior is not scattered across calls/prologue/memory without a
  clear owner.
- Focused AArch64 backend proof preserves existing variadic call behavior.

## Closure Note

Closed after the active runbook exhausted all five steps. The compiled
`variadic.cpp` and `variadic.hpp` owner exists, `variadic.md` is removed, and
the focused backend before/after proof preserved the 139-test backend subset
with no new failures. Full variadic function-entry, `va_list`, register-save
area, and overflow-area behavior remain outside this completed ownership
redistribution and require a separate capability idea.

## Reviewer Reject Signals

Reject the route or slice if it:

- implements named-case variadic shortcuts instead of general ABI-boundary
  handling;
- claims `va_start`/`va_arg` support without structured prepared facts;
- weakens variadic tests or unsupported contracts;
- leaves `variadic.md` in place after claiming reconciliation;
- mixes unrelated call, prologue, memory, or aggregate changes into this shard.
