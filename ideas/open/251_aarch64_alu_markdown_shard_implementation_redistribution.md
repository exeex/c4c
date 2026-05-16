# AArch64 `alu.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-16

## Intent

Reconcile `src/backend/mir/aarch64/codegen/alu.md` back into real compiled
owners named `alu.cpp` and `alu.hpp`. The ALU shard should have an ordinary
source/header home that owns valid ALU-family behavior instead of leaving
ALU-specific construction, lowering, or printing bodies stranded in broader
codegen files.

This is the first batch of the per-shard redistribution plan. Later ideas
should cover other `.md` shards separately; this idea is only for `alu.md`.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but `alu.cpp` is currently only partially acting as the real ALU
owner. ALU-related behavior may still be centralized in files such as
`instruction.cpp`, `dispatch.cpp`, or `machine_printer.cpp`, making the shard
layout misleading and keeping family-specific code in family-neutral owners.

## In Scope

- Move ALU-specific construction, lowering, and helper behavior from broad
  owners into `alu.cpp`/`alu.hpp` where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than ALU-family bulk implementation.
- Keep `dispatch.cpp` as routing into the ALU owner rather than a place that
  contains the ALU body.
- Keep `machine_printer.cpp` from retaining ALU-specific spelling bodies when
  those can be target-owned through ALU shard helpers.
- Delete `src/backend/mir/aarch64/codegen/alu.md` once its durable content has
  been reconciled into the compiled owner files.
- Preserve existing ALU behavior while moving ownership.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Broad AArch64 codegen cleanup unrelated to the `alu.md` shard.
- Expanding ALU feature semantics beyond what is necessary for
  behavior-preserving relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/alu.cpp` and
  `src/backend/mir/aarch64/codegen/alu.hpp` exist and own the valid ALU shard
  behavior that was previously described by `alu.md`.
- `src/backend/mir/aarch64/codegen/alu.md` is deleted before this idea closes.
- `instruction.cpp` retains only family-neutral instruction core, not
  ALU-family bulk implementation.
- `dispatch.cpp` routes to the ALU owner rather than containing ALU bodies.
- `machine_printer.cpp` does not retain ALU-specific spelling bodies when that
  spelling can be owned through ALU shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported ALU paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off ALU spellings
  instead of preserving the real semantic lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while ALU implementation remains centralized elsewhere;
- moves broad instruction, dispatch, or printer responsibilities into `alu.cpp`
  that are not ALU-family behavior;
- leaves `alu.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes ALU semantics except where strictly required for behavior-preserving
  relocation.
