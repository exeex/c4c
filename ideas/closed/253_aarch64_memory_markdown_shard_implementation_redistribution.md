# AArch64 `memory.md` Shard Implementation Redistribution

Status: Closed
Created: 2026-05-16
Closed: 2026-05-16

## Intent

Reconcile `src/backend/mir/aarch64/codegen/memory.md` back into real compiled
owners named `memory.cpp` and `memory.hpp`. The memory shard should have an
ordinary source/header home that owns valid load/store, address, and memory
operand behavior instead of leaving memory-family selected-node construction,
lowering, or spelling stranded in broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the memory shard still needs to be reconciled into compiled
owners. Load/store, address, and memory operand behavior may be centralized in
files such as `instruction.cpp`, `dispatch.cpp`, or `machine_printer.cpp`,
making the shard layout misleading and keeping family-specific code in
family-neutral owners.

## In Scope

- Move memory-specific load/store, address, memory operand selected-node
  construction, lowering, and spelling behavior from broad owners into
  `memory.cpp`/`memory.hpp` where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than memory-family bulk implementation.
- Keep `dispatch.cpp` as routing into the memory owner rather than a place that
  contains the memory-family body.
- Keep `machine_printer.cpp` from retaining memory-specific spelling bodies
  when those can be owned through memory shard helpers.
- Delete `src/backend/mir/aarch64/codegen/memory.md` once its durable content
  has been reconciled into the compiled owner files.
- Preserve existing memory behavior while moving ownership without inventing
  missing prepared memory facts.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Broad AArch64 codegen cleanup unrelated to the `memory.md` shard.
- Inventing missing prepared memory facts or expanding load/store, address, or
  memory operand semantics beyond behavior-preserving relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/memory.cpp` and
  `src/backend/mir/aarch64/codegen/memory.hpp` exist and own the valid memory
  shard behavior that was previously described by `memory.md`.
- `src/backend/mir/aarch64/codegen/memory.md` is deleted before this idea
  closes.
- `instruction.cpp` retains only family-neutral instruction core, not
  memory-family bulk implementation.
- `dispatch.cpp` routes to the memory owner rather than containing
  memory-family bodies.
- `machine_printer.cpp` does not retain memory-specific spelling bodies when
  that spelling can be owned through memory shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Closure Notes

Closed after Step 5 deleted `src/backend/mir/aarch64/codegen/memory.md` and
the closure review found no blockers. The valid durable memory-shard behavior
is represented by compiled memory owners, existing shared contracts, or
explicit deferred carrier-gap notes from `todo.md`.

Deferred carrier gaps remain out of scope for this redistribution and should
become separate ideas only when implementation is requested: missing prepared
direct-load and pointer-value-load variants, stack-slot-vs-pointer-slot
fallback behavior, unresolved-address handling, GEP and large signed-offset
materialization, dynamic-stack operations, over-aligned alloca carriers, and
bytewise memcpy lowering with explicit source, destination, size, scratch,
loop-label, alignment, and overlap semantics.

Closure proof: full build plus full CTest passed in `test_after.log`
(`3167/3167` tests), and regression guard against `test_baseline.log` passed
with no new failures using the non-decreasing pass-count mode appropriate for
behavior-preserving redistribution.

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported memory paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off load/store,
  address, or memory operand spellings instead of preserving the real semantic
  lowering/printing behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while memory implementation remains centralized
  elsewhere;
- invents missing prepared memory facts to make the redistribution appear more
  complete than the existing behavior supports;
- moves broad instruction, dispatch, or printer responsibilities into
  `memory.cpp` that are not memory-family behavior;
- leaves `memory.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes memory semantics except where strictly required for
  behavior-preserving relocation.
