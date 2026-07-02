# RV64 Prepared Local-Memory Addressing Runbook

Status: Active
Source Idea: ideas/open/555_rv64_prepared_local_memory_addressing_src_960209_1.md

## Purpose

Repair the RV64 object-route local-memory addressing capability exposed after
`src/960209-1.c` moved past missing out-of-SSA move-bundle publication.

## Goal

Make the `src/960209-1.c` row move past
`unsupported_local_memory_access` by consuming supported prepared frame-slot or
pointer-value base-plus-offset local-memory facts.

## Core Rule

Do not guess local-memory addresses from testcase shape or raw target details.
The RV64 object route must consume semantic prepared facts, or the route must
record a producer-owned missing-fact blocker for lifecycle routing.

## Read First

- `ideas/open/555_rv64_prepared_local_memory_addressing_src_960209_1.md`
- `ideas/closed/554_out_of_ssa_parallel_copy_move_bundle_publication.md`
- `ideas/open/547_bir_local_memory_call_metadata_boundary_review.md`
- `build/rv64_gcc_c_torture_backend/src_960209-1.c/case.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- prepared local-memory and frame-slot tests under `tests/backend/`

## Current Targets

- Current row: `src/960209-1.c`
- Current diagnostic: `unsupported_local_memory_access`
- Required shape from the diagnostic: prepared frame-slot or pointer-value
  base-plus-offset local memory addressing
- Current route owner: RV64 prepared object local-memory addressing, unless
  Step 1 proves required prepared facts are absent

## Non-Goals

- Do not broaden this into the full local-memory bucket from idea 547.
- Do not implement call-metadata cleanup.
- Do not route this row to F128 quarantine unless fresh row-level facts prove
  it is F128-primary.
- Do not weaken gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not special-case `src/960209-1.c`, a block label, local symbol, stack
  slot, or instruction shape.

## Working Model

- The previous first blocker, `prepared_consumer_category=missing_move_bundle`,
  is fixed for this row.
- The row now reaches RV64 object emission for a local-memory operation whose
  addressing form is not yet accepted by the object route.
- The route is RV64-owned only if prepared frame-slot or pointer-value
  base-plus-offset facts are present and coherent.

## Execution Rules

- Keep packet progress and proof commands in `todo.md`.
- Preserve the local-memory diagnostic until the addressing owner is proven.
- Prefer focused backend object-route tests before relying on the one-row
  torture scan.
- Any code-changing packet needs fresh build proof and the delegated focused
  proof command from the supervisor.
- If the first operation lacks prepared address facts, record the missing
  producer facts in `todo.md` and request lifecycle routing instead of
  guessing in RV64.

## Steps

### Step 1: Reproduce And Classify Local-Memory Facts

Goal: identify the failing local-memory operation and classify whether RV64 has
the prepared address facts it needs.

Actions:

- Re-run or inspect the one-row `src/960209-1.c` failure to confirm the
  current `unsupported_local_memory_access` diagnostic.
- Locate the object-emission branch that emits the diagnostic.
- Trace the prepared operand facts available for the failing operation:
  frame-slot base, pointer-value base, offset, value type, load/store kind, and
  any relevant home facts.
- Decide whether the first bad fact is RV64 object-route support for an
  already-published shape, or missing BIR/prepared producer facts.

Completion check:

- `todo.md` names the failing operation, available prepared facts, and first
  owner.
- No semantic repair is made until the first local-memory owner is known.

### Step 2: Implement The Owned Addressing Shape

Goal: support the prepared local-memory address form proven in Step 1.

Actions:

- Update RV64 object emission to consume the prepared frame-slot or
  pointer-value base-plus-offset form that Step 1 classified as available.
- Keep the rule generalized across local symbols, stack slots, blocks, and
  instruction shapes.
- Add or update focused backend tests for the addressing contract.
- Preserve existing rejection behavior for unsupported or missing prepared
  facts.

Completion check:

- Focused backend tests prove the owned prepared local-memory addressing form.
- The old local-memory diagnostic no longer fires for the repaired focused
  scenario unless a deeper producer-owned blocker is exposed.

### Step 3: Prove The Row Moves Past LocalMemoryAccess

Goal: verify the repaired local-memory path against `src/960209-1.c` without
weakening pass/fail accounting.

Actions:

- Run the delegated build and one-row RV64 gcc torture backend scan.
- Inspect the case log for the current first blocker.
- If the row advances to F128, call metadata, another memory producer gap, or
  another object-route blocker, record the new auditable owner in `todo.md`
  and request lifecycle routing instead of expanding this plan silently.

Completion check:

- `test_after.log` records build proof and focused row proof.
- `todo.md` states whether `unsupported_local_memory_access` is fixed or
  replaced by a different row-level first blocker.
- No expectations, unsupported markers, allowlists, or runtime comparison
  behavior were weakened.
