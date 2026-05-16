# AArch64 `returns.md` Shard Implementation Redistribution

Status: Closed
Created: 2026-05-16
Closed: 2026-05-16

Completion note: Active runbook Steps 1-5 completed. The returns markdown
shard was reconciled into compiled AArch64 returns owners,
`src/backend/mir/aarch64/codegen/returns.md` was deleted, and the broader
`^backend_aarch64_` close gate passed 27/27 with no regressions.

## Intent

Reconcile `src/backend/mir/aarch64/codegen/returns.md` back into real compiled
owners named `returns.cpp` and `returns.hpp`. The returns shard should have an
ordinary source/header home that owns valid return-value materialization and
return-control selected-node construction, lowering, and spelling behavior
instead of leaving returns-specific bodies stranded in broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the returns shard still needs to be reconciled into compiled
owners. Return-value materialization and return-control behavior may be
centralized in files such as `instruction.cpp`, `dispatch.cpp`, or
`machine_printer.cpp`, making the shard layout misleading and keeping
family-specific code in family-neutral owners.

## In Scope

- Move return-value materialization and return-control selected-node
  construction, lowering, helper, and spelling behavior from broad owners into
  `returns.cpp`/`returns.hpp` where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than returns-family bulk implementation.
- Keep `dispatch.cpp` as routing into the returns owner rather than a place
  that contains the returns-family body.
- Keep `machine_printer.cpp` from retaining returns-specific spelling bodies
  when those can be owned through returns shard helpers.
- Delete `src/backend/mir/aarch64/codegen/returns.md` once its durable content
  has been reconciled into the compiled owner files.
- Preserve existing returns behavior while moving ownership without changing
  ABI return semantics.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Redistributing `mod.md`; that file is an old index byproduct more like Rust
  `mod.rs` or Python `__init__.py`, not a normal implementation shard for this
  batch.
- Broad AArch64 codegen cleanup unrelated to the `returns.md` shard.
- Changing ABI return semantics beyond what is necessary for
  behavior-preserving relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/returns.cpp` and
  `src/backend/mir/aarch64/codegen/returns.hpp` exist and own the valid returns
  shard behavior that was previously described by `returns.md`.
- `src/backend/mir/aarch64/codegen/returns.md` is deleted before this idea
  closes.
- `instruction.cpp` retains only family-neutral instruction core, not
  returns-family bulk implementation.
- `dispatch.cpp` routes to the returns owner rather than containing
  returns-family bodies.
- `machine_printer.cpp` does not retain returns-specific spelling bodies when
  that spelling can be owned through returns shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported returns paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off return-value
  or return-control spellings instead of preserving the real semantic
  lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while returns implementation remains centralized
  elsewhere;
- changes ABI return semantics to make the redistribution appear more complete
  than the existing behavior supports;
- moves broad instruction, dispatch, or printer responsibilities into
  `returns.cpp` that are not returns-family behavior;
- leaves `returns.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes returns semantics except where strictly required for
  behavior-preserving relocation.
