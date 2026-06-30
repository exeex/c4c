# Array/Aggregate Global Layout Authority Plan

Status: Active
Source Idea: ideas/open/448_array_aggregate_global_layout_authority.md

## Purpose

Publish or reject explicit prepared array/aggregate global layout authority so
non-scalar global-symbol memory accesses remain fail-closed until producer
facts are sound.

## Goal

Classify non-scalar global memory rows such as `930930-1 @mem+792`, define the
producer facts required for array/aggregate layout authority, and select only
a bounded producer/prepared packet with focused coverage.

## Core Rule

Do not infer array or aggregate global object identity, extent,
addressability, offset meaning, element or field layout, or range authority in
RV64 from raw BIR, symbol spelling, object labels, representative filenames,
or dump shape.

## Read First

- ideas/open/448_array_aggregate_global_layout_authority.md
- ideas/closed/446_global_memory_layout_authority_publication.md
- ideas/closed/439_store_source_global_memory_publication_authority.md
- build/agent_state/446_step4_residual_disposition/classification.md
- build/agent_state/446_step4_residual_disposition/probe_summary.tsv
- build/agent_state/446_step4_residual_disposition/930930-1.prepared.out
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/bir/lir_to_bir/module.cpp
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Non-scalar prepared global-symbol memory rows from the 446 Step 4 residual
  evidence.
- `930930-1 @mem+792`, which remains `layout_authority=unknown` with
  `range_verdict=proven_in_bounds`.
- Producer facts for array/aggregate global object identity, extent,
  addressability, element or field offset, access width, alignment, range, and
  layout authority.
- Focused tests that distinguish accepted non-scalar authority from missing or
  incoherent authority.

## Non-Goals

- Scalar global layout authority already completed by idea 446.
- Immediate global-store source encoding, tracked by idea 447.
- Pointer-value memory authority, direct-global return/select-chain authority,
  terminator/select publication, or local/frame-slot residuals.
- RV64 target-side guessing from raw BIR or representative testcase shape.
- Weakening idea 439 publication predicates so unknown non-scalar layout
  authority becomes target-consumable.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, or pass/fail accounting changes.
- Accepting or modifying `test_baseline.new.log`.

## Working Model

Scalar global layout authority is complete, but non-scalar global accesses need
their own producer contract. This plan owns only the non-scalar layout
authority question: either the producer/prepared pipeline can prove enough
array/aggregate object facts for a bounded class, or those rows stay
fail-closed and any target work remains out of scope.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat non-scalar `layout_authority=unknown` as fail-closed until a
  producer-owned fact proves object identity, extent, addressability,
  element/field offset, width, alignment, and range.
- Add semantic producer/prepared coverage before any target-consumer routing.
- Select at most one narrow code packet after the contract is explicit.
- If no bounded non-scalar producer source exists, record the blocker instead
  of widening the plan into RV64 inference.
- Do not touch `test_baseline.new.log`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Non-Scalar Global Layout Evidence

Inspect the 446 Step 4 residual evidence for non-scalar prepared
global-symbol memory rows. Record each row's symbol, aggregate or array shape
evidence, offset, access width, alignment, range verdict, current layout
authority, and first missing producer fact. Completion means `todo.md`
contains a bucket table and identifies the first bounded non-scalar
layout-authority packet or exact producer blocker.

### Step 2: Define Array/Aggregate Layout Authority Contract

Specify the prepared facts required for accepted non-scalar global layout
authority, including object identity, definition/status, extent,
addressability, element or field layout, offset, width, alignment, and range.
Separate accepted records from missing or incoherent authority. Completion
means `todo.md` states the contract, rejected adjacent shapes, owned
files/tests, and proof command.

### Step 3: Implement Or Route First Non-Scalar Authority Packet

If a coherent producer/prepared packet exists, implement the smallest semantic
change with focused coverage for accepted and fail-closed cases. If the first
owner is a distinct metadata producer gap, record the split or blocker instead
of broadening this plan. Completion means proof passes or canonical lifecycle
state records the route decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative non-scalar global memory rows against the Step 3
result, classify remaining residuals, and decide whether this source idea is
complete. Completion means close, keep active with an exact remaining
array/aggregate layout-authority packet, or route durable follow-up work.
