# AArch64 `i128_ops.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/i128_ops.md` back into real
compiled owners named `i128_ops.cpp` and `i128_ops.hpp`. The i128-ops shard
should have an ordinary source/header home that owns valid i128 pair/helper
selected-node construction, lowering, and spelling instead of leaving
i128-specific bodies stranded in broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
or instruction type redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the i128-ops shard still needs to be reconciled into compiled
owners. I128 pair/helper construction, lowering, and spelling may be
centralized in files such as `instruction.cpp`, `dispatch.cpp`, or
`machine_printer.cpp`, making the shard layout misleading and keeping
family-specific code in family-neutral owners.

## In Scope

- Move i128-specific selected-node construction, lowering, helper, pair, and
  spelling behavior from broad owners into `i128_ops.cpp`/`i128_ops.hpp` where
  the behavior belongs.
- Consume existing prepared/shared i128 authority instead of inventing a new
  authority model while redistributing ownership.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than i128-family bulk implementation.
- Keep `dispatch.cpp` as routing into the i128-ops owner rather than a place
  that contains the i128-family body.
- Keep `machine_printer.cpp` from retaining i128-specific spelling bodies when
  those can be owned through i128-ops shard helpers.
- Delete `src/backend/mir/aarch64/codegen/i128_ops.md` once its durable valid
  content has been reconciled into the compiled owner files.
- Preserve existing i128 pair/helper behavior while moving ownership.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Redistributing `mod.md`; that file is an index byproduct, not a normal
  implementation redistribution shard for this batch.
- Redistributing `records.md`; that file is a stale record-pile special case,
  not a normal implementation redistribution shard for this batch.
- Broad AArch64 codegen cleanup unrelated to the `i128_ops.md` shard.
- Reintroducing fixed-register shortcuts while redistributing i128 ownership.
- Expanding i128 semantics beyond what is necessary for behavior-preserving
  relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/i128_ops.cpp` and
  `src/backend/mir/aarch64/codegen/i128_ops.hpp` exist and own the valid
  i128-ops shard behavior that was previously described by `i128_ops.md`.
- `src/backend/mir/aarch64/codegen/i128_ops.md` is deleted before this idea
  closes.
- The route consumes existing prepared/shared i128 authority and does not
  reintroduce fixed-register shortcuts.
- `instruction.cpp` retains only family-neutral instruction core, not
  i128-family bulk implementation.
- `dispatch.cpp` routes to the i128-ops owner rather than containing
  i128-family bodies.
- `machine_printer.cpp` does not retain i128-specific spelling bodies when
  that spelling can be owned through i128-ops shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported i128 paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off i128
  pair/helper spellings instead of preserving the real
  construction/lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while i128 implementation remains centralized
  elsewhere;
- reintroduces fixed-register shortcuts instead of consuming existing
  prepared/shared i128 authority;
- moves broad instruction, dispatch, or printer responsibilities into
  `i128_ops.cpp` that are not i128-family behavior;
- leaves `i128_ops.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes i128 semantics except where strictly required for
  behavior-preserving relocation.
