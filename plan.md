# Global Memory Layout Authority Publication Plan

Status: Active
Source Idea: ideas/open/446_global_memory_layout_authority_publication.md

## Purpose

Publish or reject explicit prepared global memory layout authority so global
load/store publication can become target-consumable without RV64 inference.

## Goal

Classify the remaining global-symbol memory records, define the producer facts
required for layout authority, and implement only a bounded producer/prepared
packet with focused fail-closed coverage.

## Core Rule

Do not infer global object identity, extent, addressability, offset meaning, or
layout authority in RV64 from raw BIR, symbol spelling, object labels,
representative filenames, or dump shape.

## Read First

- ideas/open/446_global_memory_layout_authority_publication.md
- ideas/closed/439_store_source_global_memory_publication_authority.md
- build/agent_state/439_step4_residual_disposition/classification.md
- build/agent_state/439_step4_residual_disposition/evidence_snippets.txt
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/bir/lir_to_bir/module.cpp
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Prepared global-symbol memory rows from the 439 Step 4 residual evidence.
- Global accesses such as `@mem+792` and `@global+0` with
  `layout_authority=unknown`.
- Producer facts for global object identity, extent, addressability, offset,
  width, alignment, range, and layout authority.
- Focused tests that distinguish coherent authority from missing or incoherent
  authority.

## Non-Goals

- Reopening selected global object-data emission completed by idea 433.
- Immediate global-store source encoding; that belongs to idea 447.
- Pointer-value memory authority, direct-global return/select-chain authority,
  or terminator/select publication.
- RV64 target-side guessing from raw BIR or representative testcase shape.
- Weakening idea 439 publication predicates so
  `layout_authority=unknown` becomes target-consumable.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, or pass/fail accounting changes.
- Accepting or modifying `test_baseline.new.log`.

## Working Model

Idea 439 made global memory publication fail closed when producer facts are not
explicit. This plan owns the missing layout-authority side only: either the
producer/prepared pipeline can publish coherent global layout authority for a
bounded class of global-symbol accesses, or the residual remains classified as
blocked without target-consumer work.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat `layout_authority=unknown` as fail-closed until a producer-owned fact
  proves global identity, extent, addressability, offset, width, alignment, and
  range.
- Add semantic producer/prepared coverage before any target-consumer routing.
- Select at most one narrow code packet after the contract is explicit.
- If no bounded producer source exists, record the blocker instead of widening
  the plan into RV64 inference.
- Do not touch `test_baseline.new.log`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Global Layout Authority Evidence

Inspect the 439 Step 4 residual evidence for prepared global-symbol memory
rows. Record each row's symbol, offset, access width, range verdict, current
layout authority, and first missing producer fact. Completion means `todo.md`
contains a bucket table and identifies the first bounded layout-authority
packet or exact producer blocker.

### Step 2: Define Global Layout Authority Contract

Specify the prepared facts required for accepted global layout authority,
including global object identity, definition/status, extent, addressability,
offset, width, alignment, and range. Separate accepted records from missing or
incoherent authority. Completion means `todo.md` states the contract, rejected
adjacent shapes, owned files/tests, and proof command.

### Step 3: Implement Or Route First Layout Authority Packet

If a coherent producer/prepared packet exists, implement the smallest semantic
change with focused coverage for accepted and fail-closed cases. If the first
owner is a distinct metadata producer gap, record the split or blocker instead
of broadening this plan. Completion means proof passes or canonical lifecycle
state records the route decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative global memory rows against the Step 3 result, classify
remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with an exact remaining layout-authority
packet, or route durable follow-up work.
