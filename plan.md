# Prepared Move-Bundle Ambiguous Stack Destination Runbook

Status: Active
Source Idea: ideas/open/556_prepared_move_bundle_ambiguous_stack_destination_src_960209_1.md
Supersedes: ideas/closed/555_rv64_prepared_local_memory_addressing_src_960209_1.md

## Purpose

Route the `src/960209-1.c` row after local-memory addressing advanced and the
first blocker became the prepared move-bundle classifier rejection for
ambiguous non-parallel multi-source stack-destination authority.

## Goal

Determine whether the row needs a producer/classifier repair or whether the
existing fail-closed classifier rejection is the correct precise owner.

## Core Rule

Do not let RV64 object emission infer source ownership, sequencing, or stack
destination authority from an ambiguous prepared move bundle. The prepared
contract must publish coherent facts or reject precisely before RV64 consumes
the bundle.

## Read First

- `ideas/open/556_prepared_move_bundle_ambiguous_stack_destination_src_960209_1.md`
- `ideas/closed/555_rv64_prepared_local_memory_addressing_src_960209_1.md`
- `ideas/closed/516_rv64_multi_source_prepared_move_bundle_classification.md`
- `ideas/closed/552_prepared_move_bundle_target_shape_authority_gaps.md`
- `build/rv64_gcc_c_torture_backend/src_960209-1.c/case.log`
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `src/backend/prealloc/prepared_object_traversal.hpp`
- `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Current row: `src/960209-1.c`
- Current diagnostic:
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
- Current owner: prepared move-bundle producer/classifier contract before RV64
  object emission

## Non-Goals

- Do not reopen RV64 local-memory addressing unless fresh evidence proves this
  classifier blocker masked a still-present local-memory failure.
- Do not materialize ambiguous move bundles in RV64 by choosing a source,
  dropping a move, or inventing ordering.
- Do not implement broad parallel-copy scheduling.
- Do not weaken gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not special-case `src/960209-1.c`, a block label, value name, stack slot,
  or instruction shape.

## Working Model

- The local-memory route is complete for this row.
- Existing prepared classifier coverage intentionally rejects ambiguous
  non-parallel multi-source stack-destination bundles.
- This row needs evidence to decide whether its bundle is a legitimate
  fail-closed case or whether a producer should publish stronger sequencing,
  split-move, or parallel-copy authority.

## Execution Rules

- Keep packet progress and proof commands in `todo.md`.
- Preserve the classifier diagnostic until the owner is proven.
- Prefer focused prepared/classifier tests before relying on the one-row
  torture scan.
- Any code-changing packet needs fresh build proof and the delegated focused
  proof command from the supervisor.
- If the classifier rejection is retained as correct, record the row-level
  facts that make it precise rather than unexplained.

## Steps

### Step 1: Reproduce And Capture Bundle Facts

Goal: identify the first ambiguous move bundle for `src/960209-1.c` and record
the facts the classifier sees.

Actions:

- Re-run or inspect the one-row `src/960209-1.c` failure and confirm the
  current classifier diagnostic.
- Locate the prepared traversal event and classifier branch that emits the
  rejection.
- Record source homes, destination home, destination stack slot, scalar type
  and width, move count, ordering, `parallel_copy`, and authority facts.
- Compare the shape with existing focused ambiguous stack-destination tests.

Completion check:

- `todo.md` names the first ambiguous bundle, available facts, and whether the
  row matches the existing fail-closed contract or exposes a missing producer
  authority.

### Step 2: Repair Or Justify The Prepared Contract

Goal: implement the minimal semantic fix if Step 1 proves the prepared contract
is too weak, or preserve the precise rejection if it is correct.

Actions:

- If producer facts are missing, update the producer/classifier boundary to
  publish coherent sequencing, split the bundle, or record explicit authority.
- If the rejection is correct, add focused coverage and notes proving why RV64
  must not consume the bundle.
- Keep the rule generalized across rows, value names, slots, and instruction
  shapes.
- Preserve existing rejection behavior for truly ambiguous non-parallel
  multi-source stack-destination bundles.

Completion check:

- Focused prepared/classifier coverage proves the accepted repair or retained
  rejection.
- The same unexplained classifier blocker is no longer claimed as progress.

### Step 3: Prove The Row-Level Outcome

Goal: verify the chosen prepared contract against `src/960209-1.c` without
weakening pass/fail accounting.

Actions:

- Run the delegated build and one-row RV64 gcc torture backend scan.
- Inspect the case log for the current first blocker.
- If the row advances to a new owner, record the auditable diagnostic in
  `todo.md` and request lifecycle routing instead of expanding this plan
  silently.

Completion check:

- `test_after.log` records build proof and focused row proof.
- `todo.md` states whether the ambiguous stack-destination blocker was
  repaired, retained as precise, or replaced by a different row-level first
  blocker.
- No expectations, unsupported markers, allowlists, or runtime comparison
  behavior were weakened.
