# AArch64 `float_ops.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/float_ops.md` back into real
compiled owners named `float_ops.cpp` and `float_ops.hpp`. The float-ops shard
should have an ordinary source/header home that owns valid scalar floating
operation selected-node construction, lowering, and spelling instead of
leaving float-specific bodies stranded in broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
or instruction type redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the float-ops shard still needs to be reconciled into compiled
owners. Scalar floating operation construction, lowering, and spelling may be
centralized in files such as `instruction.cpp`, `dispatch.cpp`, or
`machine_printer.cpp`, making the shard layout misleading and keeping
family-specific code in family-neutral owners.

## In Scope

- Move float-specific selected-node construction, lowering, helper, and
  spelling behavior from broad owners into `float_ops.cpp`/`float_ops.hpp`
  where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than float-family bulk implementation.
- Keep `dispatch.cpp` as routing into the float-ops owner rather than a place
  that contains the float-family body.
- Keep `machine_printer.cpp` from retaining float-specific spelling bodies
  when those can be owned through float-ops shard helpers.
- Delete `src/backend/mir/aarch64/codegen/float_ops.md` once its durable valid
  content has been reconciled into the compiled owner files.
- Preserve existing scalar floating operation behavior while moving ownership.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Redistributing `mod.md`; that file is an index byproduct, not a normal
  implementation redistribution shard for this batch.
- Redistributing `records.md`; that file is a stale record-pile special case,
  not a normal implementation redistribution shard for this batch.
- Broad AArch64 codegen cleanup unrelated to the `float_ops.md` shard.
- Inventing missing floating-point prepared facts while redistributing the
  shard.
- Expanding floating-point semantics beyond what is necessary for
  behavior-preserving relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/float_ops.cpp` and
  `src/backend/mir/aarch64/codegen/float_ops.hpp` exist and own the valid
  float-ops shard behavior that was previously described by `float_ops.md`.
- `src/backend/mir/aarch64/codegen/float_ops.md` is deleted before this idea
  closes.
- `instruction.cpp` retains only family-neutral instruction core, not
  float-family bulk implementation.
- `dispatch.cpp` routes to the float-ops owner rather than containing
  float-family bodies.
- `machine_printer.cpp` does not retain float-specific spelling bodies when
  that spelling can be owned through float-ops shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported floating paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off scalar
  floating operation spellings instead of preserving the real
  construction/lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while float-ops implementation remains centralized
  elsewhere;
- invents missing FP prepared facts or expands floating semantics beyond
  behavior-preserving relocation;
- moves broad instruction, dispatch, or printer responsibilities into
  `float_ops.cpp` that are not float-family behavior;
- leaves `float_ops.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes floating operation semantics except where strictly required for
  behavior-preserving relocation.
