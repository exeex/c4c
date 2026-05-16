# AArch64 `calls.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-16

## Intent

Reconcile `src/backend/mir/aarch64/codegen/calls.md` back into real compiled
owners named `calls.cpp` and `calls.hpp`. The calls shard should have an
ordinary source/header home that owns valid call-family behavior instead of
leaving call lowering, ABI-adjacent selected call records, helper-call printing
bodies, or call-family machine-node construction and spelling stranded in
broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the calls shard still needs to be reconciled into compiled
owners. Call-specific behavior may be centralized in files such as
`instruction.cpp`, `dispatch.cpp`, or `machine_printer.cpp`, making the shard
layout misleading and keeping family-specific code in family-neutral owners.

## In Scope

- Move call-specific lowering, selected-node construction, helper-call
  printing, and spelling behavior from broad owners into `calls.cpp`/`calls.hpp`
  where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than call-family bulk implementation.
- Keep `dispatch.cpp` as routing into the calls owner rather than a place that
  contains the call-family body.
- Keep `machine_printer.cpp` from retaining call-specific spelling bodies when
  those can be owned through calls shard helpers.
- Delete `src/backend/mir/aarch64/codegen/calls.md` once its durable content
  has been reconciled into the compiled owner files.
- Preserve existing call behavior while moving ownership.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Broad AArch64 codegen cleanup unrelated to the `calls.md` shard.
- Expanding call or ABI semantics beyond what is necessary for
  behavior-preserving relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/calls.cpp` and
  `src/backend/mir/aarch64/codegen/calls.hpp` exist and own the valid calls
  shard behavior that was previously described by `calls.md`.
- `src/backend/mir/aarch64/codegen/calls.md` is deleted before this idea
  closes.
- `instruction.cpp` retains only family-neutral instruction core, not
  call-family bulk implementation.
- `dispatch.cpp` routes to the calls owner rather than containing call-family
  bodies.
- `machine_printer.cpp` does not retain call-specific spelling bodies when
  that spelling can be owned through calls shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported call paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off call
  spellings instead of preserving the real semantic lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while call implementation remains centralized elsewhere;
- moves broad instruction, dispatch, or printer responsibilities into
  `calls.cpp` that are not call-family behavior;
- leaves `calls.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes call semantics except where strictly required for behavior-preserving
  relocation.
