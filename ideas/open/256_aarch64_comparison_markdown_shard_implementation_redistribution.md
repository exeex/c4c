# AArch64 `comparison.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-16

## Intent

Reconcile `src/backend/mir/aarch64/codegen/comparison.md` back into real
compiled owners named `comparison.cpp` and `comparison.hpp`. The comparison
shard should have an ordinary source/header home that owns valid compare,
condition, and branch-control selected-node construction, lowering, and
spelling behavior instead of leaving comparison-specific bodies stranded in
broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the comparison shard still needs to be reconciled into compiled
owners. Compare, condition, and branch-control behavior may be centralized in
files such as `instruction.cpp`, `dispatch.cpp`, or `machine_printer.cpp`,
making the shard layout misleading and keeping family-specific code in
family-neutral owners.

## In Scope

- Move comparison-specific selected-node construction, lowering, helper, and
  spelling behavior from broad owners into `comparison.cpp`/`comparison.hpp`
  where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than comparison-family bulk implementation.
- Keep `dispatch.cpp` as routing into the comparison owner rather than a place
  that contains the comparison-family body.
- Keep `machine_printer.cpp` from retaining comparison-specific spelling
  bodies when those can be owned through comparison shard helpers.
- Delete `src/backend/mir/aarch64/codegen/comparison.md` once its durable
  content has been reconciled into the compiled owner files.
- Preserve existing comparison behavior while moving ownership.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Redistributing `mod.md`; that file is an old index byproduct more like Rust
  `mod.rs` or Python `__init__.py`, not a normal implementation shard for this
  batch.
- Broad AArch64 codegen cleanup unrelated to the `comparison.md` shard.
- Expanding condition semantics beyond what is necessary for
  behavior-preserving relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/comparison.cpp` and
  `src/backend/mir/aarch64/codegen/comparison.hpp` exist and own the valid
  comparison shard behavior that was previously described by `comparison.md`.
- `src/backend/mir/aarch64/codegen/comparison.md` is deleted before this idea
  closes.
- `instruction.cpp` retains only family-neutral instruction core, not
  comparison-family bulk implementation.
- `dispatch.cpp` routes to the comparison owner rather than containing
  comparison-family bodies.
- `machine_printer.cpp` does not retain comparison-specific spelling bodies
  when that spelling can be owned through comparison shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported comparison paths unsupported,
  or weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off
  compare/condition/branch-control spellings instead of preserving the real
  semantic lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while comparison implementation remains centralized
  elsewhere;
- expands condition semantics or branch-control behavior beyond
  behavior-preserving relocation;
- moves broad instruction, dispatch, or printer responsibilities into
  `comparison.cpp` that are not comparison-family behavior;
- leaves `comparison.md` in place after claiming the shard has a compiled
  owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes comparison semantics except where strictly required for
  behavior-preserving relocation.
