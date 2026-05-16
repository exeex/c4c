# AArch64 `globals.md` Shard Implementation Redistribution

Status: Closed
Created: 2026-05-16
Closed: 2026-05-16

## Intent

Reconcile `src/backend/mir/aarch64/codegen/globals.md` back into real compiled
owners named `globals.cpp` and `globals.hpp`. The globals shard should have an
ordinary source/header home that owns valid global address and materialization
behavior instead of leaving global selected-node construction, lowering, or
spelling stranded in broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the globals shard still needs to be reconciled into compiled
owners. Global address and materialization behavior may be centralized in files
such as `instruction.cpp`, `dispatch.cpp`, or `machine_printer.cpp`, making the
shard layout misleading and keeping family-specific code in family-neutral
owners.

## In Scope

- Move global address and materialization selected-node construction, lowering,
  and spelling behavior from broad owners into `globals.cpp`/`globals.hpp`
  where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than globals-family bulk implementation.
- Keep `dispatch.cpp` as routing into the globals owner rather than a place
  that contains the globals-family body.
- Keep `machine_printer.cpp` from retaining globals-specific spelling bodies
  when those can be owned through globals shard helpers.
- Delete `src/backend/mir/aarch64/codegen/globals.md` once its durable content
  has been reconciled into the compiled owner files.
- Preserve existing globals behavior while moving ownership without
  hard-coding symbol semantics outside prepared/shared authority.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Broad AArch64 codegen cleanup unrelated to the `globals.md` shard.
- Hard-coding symbol semantics outside prepared/shared authority or expanding
  global address/materialization semantics beyond behavior-preserving
  relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/globals.cpp` and
  `src/backend/mir/aarch64/codegen/globals.hpp` exist and own the valid globals
  shard behavior that was previously described by `globals.md`.
- `src/backend/mir/aarch64/codegen/globals.md` is deleted before this idea
  closes.
- `instruction.cpp` retains only family-neutral instruction core, not
  globals-family bulk implementation.
- `dispatch.cpp` routes to the globals owner rather than containing
  globals-family bodies.
- `machine_printer.cpp` does not retain globals-specific spelling bodies when
  that spelling can be owned through globals shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Closure Notes

The globals markdown shard was reconciled into compiled `globals.cpp` and
`globals.hpp` owners, then `src/backend/mir/aarch64/codegen/globals.md` was
deleted. Address-materialization construction, lowering, and globals-specific
printer spelling now live behind the globals owner while `instruction.cpp`,
`dispatch.cpp`, and `machine_printer.cpp` retain their generic responsibilities.

Focused close guard passed for
`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_machine_printer`, and
`backend_aarch64_instruction_dispatch`. Supervisor acceptance context reported
full `cmake --build --preset default && ctest --test-dir build -j --output-on-failure`
passed 3167/3167 before commit `f228c11a6`.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported globals paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off global
  address/materialization spellings instead of preserving the real semantic
  lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while globals implementation remains centralized
  elsewhere;
- hard-codes symbol semantics outside prepared/shared authority to make the
  redistribution appear more complete than the existing behavior supports;
- moves broad instruction, dispatch, or printer responsibilities into
  `globals.cpp` that are not globals-family behavior;
- leaves `globals.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes globals semantics except where strictly required for
  behavior-preserving relocation.
