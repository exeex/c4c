# Immediate Global-Store Source Encoding Plan

Status: Active
Source Idea: ideas/open/447_immediate_global_store_source_encoding.md

## Purpose

Publish or reject explicit prepared immediate-source encoding for global stores
so store-global publication authority can distinguish immediates from unknown
sources before RV64 lowering.

## Goal

Classify global-store rows with immediate values, define the prepared source
encoding required for immediate publication, and select only a bounded
producer/prepared packet with focused fail-closed coverage.

## Core Rule

Do not infer immediate store values, value type, bit pattern, destination
global, or publication intent in RV64 from raw BIR store shape, testcase names,
block labels, symbol spelling, or one prepared dump layout.

## Read First

- ideas/open/447_immediate_global_store_source_encoding.md
- ideas/closed/439_store_source_global_memory_publication_authority.md
- ideas/closed/446_global_memory_layout_authority_publication.md
- ideas/closed/448_array_aggregate_global_layout_authority.md
- build/agent_state/439_step4_residual_disposition/classification.md
- build/agent_state/439_step4_residual_disposition/evidence_snippets.txt
- build/agent_state/446_step4_residual_disposition/classification.md
- build/agent_state/448_step4_residual_disposition/classification.md
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/bir/lir_to_bir/module.cpp
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Store-global rows with immediate values from the 439 Step 4 residual
  evidence.
- `20041112-1 main.entry.0`, where an immediate `1` is stored to `@global+0`
  but the prepared store-source row reports `source=<none>` and
  `source_producer=unknown`.
- Prepared facts for immediate value type, width, signedness or exact bit
  pattern, destination global, and publication intent.
- Focused tests that distinguish explicit immediate-source publication from
  missing or incoherent source facts.

## Non-Goals

- Scalar global layout authority completed by idea 446.
- Integer-array global layout authority completed by idea 448.
- RV64 target-side guessing of immediate values from raw BIR store shape.
- Pointer-value memory authority, direct-global return/select-chain authority,
  terminator/select publication, or local/frame-slot residuals.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, or pass/fail accounting changes.
- Accepting or modifying `test_baseline.new.log`.

## Working Model

Global layout authority is now represented by prior producer/prepared work, but
an immediate global store still cannot be consumed while its store source is
unknown. This plan owns only the immediate-source encoding side: either the
producer/prepared pipeline publishes an explicit immediate source fact for a
bounded class of global stores, or the row remains fail-closed with a precise
producer blocker.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat `source=<none>` or `source_producer=unknown` as fail-closed until a
  producer-owned immediate source fact is explicit.
- Preserve idea 439 store-publication predicates; do not weaken them to accept
  unknown sources.
- Add semantic producer/prepared coverage before any target-consumer routing.
- Select at most one narrow code packet after the source contract is explicit.
- If no bounded immediate-source producer exists, record the blocker instead
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

### Step 1: Audit Immediate Global-Store Evidence

Inspect the 439 Step 4 residual evidence and any fresh representative dumps
needed by the executor for immediate-valued global stores. Record each row's
function/block, destination global, offset, store width, current layout
authority, immediate value, current source encoding, and first missing
producer fact. Completion means `todo.md` contains a bucket table and
identifies the first bounded immediate-source packet or exact producer blocker.

### Step 2: Define Immediate Source Encoding Contract

Specify the prepared facts required for accepted immediate global-store source
publication, including value type, width, signedness or exact bit pattern,
destination global, and publication intent. Separate accepted records from
missing or incoherent source facts. Completion means `todo.md` states the
contract, rejected adjacent shapes, owned files/tests, and proof command.

### Step 3: Implement Or Route First Immediate Source Packet

If a coherent producer/prepared packet exists, implement the smallest semantic
change with focused coverage for accepted and fail-closed cases. If the first
owner is a distinct metadata producer gap, record the split or blocker instead
of broadening this plan. Completion means proof passes or canonical lifecycle
state records the route decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative immediate global-store rows against the Step 3 result,
classify remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with an exact remaining immediate-source
packet, or route durable follow-up work.
