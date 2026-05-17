# AArch64 `f128.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/f128.md` back into real compiled
owners named `f128.cpp` and `f128.hpp`. The f128 shard should have an ordinary
source/header home that owns valid binary128/f128 transport and helper
selected-node construction, lowering, and spelling instead of leaving
f128-specific bodies stranded in broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
or instruction type redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the f128 shard still needs to be reconciled into compiled
owners. Binary128/f128 transport, helper construction, lowering, and spelling
may be centralized in files such as `instruction.cpp`, `dispatch.cpp`, or
`machine_printer.cpp`, making the shard layout misleading and keeping
family-specific code in family-neutral owners.

## In Scope

- Move f128-specific selected-node construction, lowering, helper, transport,
  and spelling behavior from broad owners into `f128.cpp`/`f128.hpp` where the
  behavior belongs.
- Preserve existing fail-closed and deferred-helper boundaries while moving
  ownership.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than f128-family bulk implementation.
- Keep `dispatch.cpp` as routing into the f128 owner rather than a place that
  contains the f128-family body.
- Keep `machine_printer.cpp` from retaining f128-specific spelling bodies when
  those can be owned through f128 shard helpers.
- Delete `src/backend/mir/aarch64/codegen/f128.md` once its durable valid
  content has been reconciled into the compiled owner files.
- Preserve existing binary128/f128 behavior while moving ownership.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Redistributing `mod.md`; that file is an index byproduct, not a normal
  implementation redistribution shard for this batch.
- Redistributing `records.md`; that file is a stale record-pile special case,
  not a normal implementation redistribution shard for this batch.
- Broad AArch64 codegen cleanup unrelated to the `f128.md` shard.
- Expanding binary128/f128 semantics beyond what is necessary for
  behavior-preserving relocation.
- Converting fail-closed or deferred-helper boundaries into new live behavior
  as part of redistribution.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/f128.cpp` and
  `src/backend/mir/aarch64/codegen/f128.hpp` exist and own the valid f128 shard
  behavior that was previously described by `f128.md`.
- `src/backend/mir/aarch64/codegen/f128.md` is deleted before this idea closes.
- Existing fail-closed and deferred-helper boundaries are preserved unless a
  separate approved source idea changes that intent.
- `instruction.cpp` retains only family-neutral instruction core, not
  f128-family bulk implementation.
- `dispatch.cpp` routes to the f128 owner rather than containing f128-family
  bodies.
- `machine_printer.cpp` does not retain f128-specific spelling bodies when
  that spelling can be owned through f128 shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported f128 paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off
  binary128/f128 spellings instead of preserving the real
  construction/lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while f128 implementation remains centralized
  elsewhere;
- converts fail-closed or deferred-helper f128 boundaries into new behavior
  without a separate approved source idea;
- moves broad instruction, dispatch, or printer responsibilities into
  `f128.cpp` that are not f128-family behavior;
- leaves `f128.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes f128 semantics except where strictly required for
  behavior-preserving relocation.
