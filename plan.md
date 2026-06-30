# Prepared Pointer-Value Memory Authority Plan

Status: Active
Source Idea: ideas/open/438_prepared_pointer_value_memory_authority.md

## Purpose

Establish explicit producer/prepared authority for pointer-value memory accesses
before any RV64 lowering consumes them.

## Goal

Classify pointer-value memory residuals by missing versus coherent prepared
authority, define the required layout/provenance/range facts, and add focused
producer coverage that keeps missing or incoherent authority fail-closed.

## Core Rule

Do not infer pointer-value memory layout, provenance, range, bounds, or object
ownership in RV64 from raw pointer values, integer casts, filenames, function
names, object labels, or dump shape.

## Read First

- ideas/open/438_prepared_pointer_value_memory_authority.md
- build/agent_state/433_step4_residual_disposition/classification.md
- build/agent_state/433_step4_residual_disposition/evidence_snippets.txt
- build/agent_state/433_step4_residual_disposition/930930-1.prepared.out
- build/agent_state/433_step4_residual_disposition/930930-1.object.err
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Pointer-value memory accesses in the `930930-1` residual evidence.
- Prepared access records with `base=pointer_value`,
  `layout_authority=unknown`, and `range_verdict=unknown_compatible`.
- Pointer homes with `kind=pointer_base_plus_offset` that may or may not carry
  enough authority for later lowering.
- Existing producer logic for pointer-value memory provenance and fail-closed
  diagnostics.

## Non-Goals

- Reopening bounded `inttoptr`/`ptrtoint` pointer-cast movement from idea 429.
- Reopening selected global object-data support completed by idea 433.
- Implementing RV64 target lowering for pointer-value memory before prepared
  authority is explicit.
- Store-source/global-memory publication, direct-global return/select-chain,
  or terminator/select publication work.
- Expectation rewrites, allowlists, unsupported downgrades, runtime-comparison
  changes, or pass/fail accounting changes.

## Working Model

The current residual is a producer/prepared authority problem until evidence
shows otherwise. The first packet should determine whether pointer-value memory
can be represented with existing prepared provenance facts, whether those facts
need repair, or whether the residual must split again into a narrower producer
initiative. RV64 lowering may only follow after the fact contract is explicit
and covered.

## Execution Rules

- Start with classification and contract review; do not edit RV64 lowering in
  Step 1.
- Keep `layout_authority=unknown` and `range_verdict=unknown_compatible`
  fail-closed unless the producer proves a stronger fact.
- Add tests for semantic producer facts, not for a representative filename.
- If implementation reaches shared BIR/prepared producer code, run focused
  producer tests plus backend proof chosen by the supervisor.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^(backend_|frontend_hir_tests$|positive_sema_)'
git diff --check
```

## Steps

### Step 1: Audit Pointer-Value Memory Evidence And Existing Authority

Inspect the Step 4 residual artifacts and the existing BIR/prepared provenance
logic. Identify each pointer-value access, its pointer home, layout authority,
range verdict, source value, and first missing fact. Completion means
`todo.md` records a bucket table and a concrete next packet: producer repair,
producer coverage, target-consumer follow-up, or split.

### Step 2: Define The Prepared Authority Contract

Specify the minimum facts needed for pointer-value memory ownership, layout,
range, and base-plus-offset validity. Compare those facts with current prepared
records and diagnostics. Completion means `todo.md` states the accepted
contract, rejected adjacent shapes, required tests, and whether the current
source idea can implement the producer packet directly.

### Step 3: Implement Or Route Producer Coverage

If the contract can be repaired locally, add focused producer/prepared tests
and the smallest implementation needed to publish or reject the authority. If
the evidence points to a distinct producer family, create or request a separate
source idea instead of widening this plan. Completion means proof passes or
canonical lifecycle state records the split.

### Step 4: Residual Disposition And Close Readiness

Re-probe the representative residuals, classify any remaining pointer-value
memory blockers, and decide whether the source idea is complete. Completion
means the idea closes, remains active with an exact remaining producer packet,
or routes a durable follow-up.
