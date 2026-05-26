# AArch64 Store Source Publication Planning Runbook

Status: Active
Source Idea: ideas/open/34_aarch64_store_source_publication_planning.md

## Purpose

Move target-neutral store-source selection facts out of AArch64 memory emission
and into shared prepare, BIR, or MIR publication planning.

## Goal

AArch64 store lowering consumes prepared store-source facts instead of
rediscovering semantic source choices from local producer shape.

## Core Rule

Publish store-source authority upstream before emission; do not replace
`memory_store_sources.*` with another AArch64-local rediscovery path.

## Read First

- `ideas/open/34_aarch64_store_source_publication_planning.md`
- `src/backend/mir/aarch64/codegen/memory_store_sources.cpp`
- `src/backend/mir/aarch64/codegen/memory_store_sources.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/bir/lir_to_bir/memory/`
- `src/backend/prealloc/prepared_lookups.*`
- `tests/backend/mir/backend_store_source_publication_plan_test.cpp`
- `tests/backend/mir/backend_x86_store_source_publication_plan_reuse_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`

## Current Targets

- Audit source-selection decisions currently inferred by
  `memory_store_sources.*`.
- Choose one concrete target-neutral store-source decision to migrate first.
- Represent that decision as a shared prepared or MIR/BIR planning fact before
  AArch64 codegen consumes it.
- Convert the matching AArch64 path to fail closed when the shared fact is
  absent or incomplete.
- Add focused positive and negative coverage for the prepared-source contract.

## Non-Goals

- Do not merge `memory_store_sources.*` into `memory.cpp`.
- Do not perform mechanical AArch64 memory file consolidation.
- Do not rewrite unrelated load/store spelling, frame layout, dynamic stack
  adjustment, or memory operation ownership.
- Do not add AArch64-only source rediscovery under a new helper name.
- Do not broaden BIR or MIR representation beyond the selected store-source
  publication fact.

## Working Model

- `memory_store_sources.*` may still contain AArch64 emission mechanics while
  this runbook is active.
- Semantic source choices should move toward shared prepared publication,
  BIR/MIR facts, or query helpers that can be reused by more than one target.
- AArch64 should be a consumer of those facts and should report explicit
  unsupported/unprepared reasons when the facts are missing.

## Execution Rules

- Keep each packet behavior-preserving except for intentional fail-closed
  handling of unprepared source facts.
- Prefer existing prepared/publication-plan types and lookup helpers before
  adding a new data path.
- Tests must prove the published fact exists before AArch64 emission consumes
  it, not just that one final assembly case changed.
- Reject testcase-shaped matching on fixture names, value ids, stack slot ids,
  instruction order, or producer labels.
- Escalate to broader backend validation before treating this idea as complete,
  because the selected fact may affect shared prepared state.

## Ordered Steps

### Step 1: Audit AArch64 Store-Source Decisions

Goal: identify the semantic decisions inside `memory_store_sources.*` and pick
one migration target with clear shared ownership.

Primary target: `src/backend/mir/aarch64/codegen/memory_store_sources.*`

Actions:

- List every source-selection category currently inferred in
  `memory_store_sources.*`, including narrow-store recovery, byval frame-slot
  load sources, cast/select/scalar-FP producer publication, direct global-load
  select chains, and pending store-global stack publications when present.
- For each category, record whether a shared prepared, BIR, MIR, or
  publication-plan fact already exists.
- Select the first migration target that can be represented upstream without
  broad representation churn.
- Write executor notes in `todo.md` only; do not edit the source idea for
  routine findings.

Completion check:

- `todo.md` names the selected first migration target, its current AArch64
  helper path, the intended shared authority, and the narrow proof command the
  executor used for audit-only validation.

### Step 2: Publish The Selected Store-Source Fact Upstream

Goal: add or extend the shared prepared/MIR/BIR fact for the selected
store-source decision.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/`
- `src/backend/prealloc/prepared_lookups.*`
- existing MIR/publication-plan types used by store-source tests

Actions:

- Reuse an existing publication-plan or prepared lookup structure when it can
  express the selected decision truthfully.
- Add the smallest shared representation needed for the selected source fact.
- Ensure the fact is available before target codegen and does not require
  AArch64 to inspect same-block producers to recover it.
- Preserve explicit absence states for unsupported or unprepared cases.

Completion check:

- A focused shared test shows the selected source fact is published before
  AArch64 emission, and the build target covering that test passes.

### Step 3: Convert AArch64 To Consume Shared Authority

Goal: replace local rediscovery for the selected decision with consumption of
the shared prepared/MIR/BIR fact.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory_store_sources.*`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- AArch64 prepared memory operand record helpers as needed

Actions:

- Route the selected store lowering path through the shared fact.
- Remove or narrow only the matching local rediscovery logic.
- Keep AArch64 emission mechanics in AArch64 files.
- Return explicit failure/unsupported records when the shared fact is missing,
  inconsistent, or incomplete.

Completion check:

- AArch64 no longer derives the selected source from same-block producer shape
  at emission time, and targeted AArch64 memory/store tests pass.

### Step 4: Prove Positive And Negative Contracts

Goal: cover both the accepted prepared-source path and the fail-closed path.

Primary targets:

- `tests/backend/mir/backend_store_source_publication_plan_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
- relevant backend route or case tests for the selected fact

Actions:

- Add positive coverage that observes the shared source fact before AArch64
  consumes it.
- Add negative coverage where AArch64 cannot proceed without the shared fact.
- Avoid expectation rewrites that merely weaken current supported contracts.

Completion check:

- The positive and negative tests fail for the old source-rediscovery route
  and pass with the shared publication route.

### Step 5: Identify Remaining AArch64 Memory Residue

Goal: leave a clear boundary for later mechanical memory fold-back cleanup.

Primary target: `todo.md`

Actions:

- Summarize which `memory_store_sources.*` responsibilities remain
  AArch64-only emission mechanics.
- Note any target-neutral source decisions still pending for future work.
- Do not start the mechanical fold-back covered by idea 39.

Completion check:

- `todo.md` contains a concise residue summary, and the remaining helper scope
  is distinguishable from the migrated semantic store-source authority.

### Step 6: Acceptance Validation

Goal: prove the migrated fact did not regress nearby backend behavior.

Actions:

- Run the narrow tests touched by Steps 2 through 4.
- Run the supervisor-selected broader backend validation checkpoint before
  closure or milestone treatment.
- Confirm no unsupported expectation downgrades, named-case shortcuts, or
  AArch64-local replacement fact table were introduced.

Completion check:

- `test_after.log` or supervisor-provided regression logs record the accepted
  proof set, and the source idea acceptance criteria are satisfied for at least
  one concrete store-source decision.
