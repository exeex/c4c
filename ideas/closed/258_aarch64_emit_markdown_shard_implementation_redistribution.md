# AArch64 `emit.md` Shard Implementation Redistribution

Status: Closed
Created: 2026-05-16

## Intent

Reconcile `src/backend/mir/aarch64/codegen/emit.md` back into real compiled
owners named `emit.cpp` and `emit.hpp`. The emit shard should have an ordinary
source/header home that owns valid top-level module emit/build orchestration
behavior instead of leaving emit-specific orchestration stranded in broader
codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
redesign.

## Why This Exists

The AArch64 codegen markdown shards describe implementation boundaries, but
the emit shard still needs to be reconciled into compiled owners. Top-level
module emit/build orchestration may be centralized in files such as
`instruction.cpp`, `dispatch.cpp`, or `machine_printer.cpp`, making the shard
layout misleading and keeping shard-specific ownership in family-neutral
owners.

`emit.cpp` should remain thin. This idea exists to give durable emit/build
orchestration an ordinary compiled home, not to create a broad catch-all for
unrelated AArch64 codegen behavior.

## In Scope

- Move top-level module emit/build orchestration behavior from broad owners
  into `emit.cpp`/`emit.hpp` where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than emit-specific orchestration.
- Keep `dispatch.cpp` as routing into the relevant shard owner rather than a
  place that contains family bodies.
- Keep `machine_printer.cpp` from retaining shard-specific spelling bodies
  when those can be owned through shard helpers; emit should coordinate rather
  than absorb printer spellings.
- Delete `src/backend/mir/aarch64/codegen/emit.md` once its durable valid
  content has been reconciled into `emit.cpp`/`emit.hpp` or explicitly
  classified as stale/out-of-scope inside the close ledger.
- Preserve existing emit/build behavior while moving ownership.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Redistributing `mod.md`; that file is an old index byproduct more like Rust
  `mod.rs` or Python `__init__.py`, not a normal implementation shard for this
  batch.
- Broad AArch64 codegen cleanup unrelated to the `emit.md` shard.
- Letting `emit.cpp` become a catch-all for selected-node families, instruction
  spelling, or backend semantics that belong in narrower shard owners.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/emit.cpp` and
  `src/backend/mir/aarch64/codegen/emit.hpp` exist and own the valid emit shard
  behavior that was previously described by `emit.md`.
- `src/backend/mir/aarch64/codegen/emit.md` is deleted before this idea closes,
  with any durable valid content reconciled into `emit.cpp`/`emit.hpp` and any
  stale or out-of-scope content explicitly classified in the close ledger.
- `instruction.cpp` retains only family-neutral instruction core, not
  emit-specific orchestration or shard-family bulk implementation.
- `dispatch.cpp` routes to shard owners rather than containing family bodies.
- `machine_printer.cpp` does not retain shard-specific spelling bodies when
  that spelling can be owned through shard helpers, and `emit.cpp` does not
  absorb those spelling bodies as a catch-all.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported emit/build paths unsupported,
  or weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off emit/build
  behavior instead of preserving the real orchestration behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while emit/build ownership remains centralized
  elsewhere;
- lets `emit.cpp` become a broad catch-all for selected-node families,
  instruction spelling, or backend semantics that belong in narrower shard
  owners;
- moves broad instruction, dispatch, or printer responsibilities into
  `emit.cpp` that are not thin emit/build orchestration;
- leaves `emit.md` in place after claiming the shard has a compiled owner;
- deletes `emit.md` without reconciling durable valid content or recording
  stale/out-of-scope classification in the close ledger;
- restores the legacy centralized record pile behind a new abstraction name;
- changes emit/build semantics except where strictly required for
  behavior-preserving relocation.

## Close Ledger

Closed: 2026-05-16

The active runbook reconciled the durable valid `emit.md` material into compiled
AArch64 MIR codegen owners and deleted
`src/backend/mir/aarch64/codegen/emit.md`. `emit.cpp`/`emit.hpp` now own the
thin prepared-module emit/build orchestration, adjacent instruction, dispatch,
and printer owners retained their narrower responsibilities, and stale legacy
direct-text emitter details were left out of the compiled surface.

Close proof used the focused AArch64 backend scope selected for the completed
slice: `ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_'`.
The Step 5 repair passed 27/27 after removing the stale source-text audit of the
deleted markdown shard. The supervisor-reported full-suite hook baseline after
Step 5 passed 3167/3167.
