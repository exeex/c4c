# BIR Semantic Local-Address Provenance And Array-Element Access Authority Plan

Status: Active
Source Idea: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Activated From: ideas/closed/483_bir_semantic_local_memory_scalar_load_producer.md

## Purpose

Provide the prerequisite local-address and array-element provenance authority
that blocked the scalar local-load producer route.

## Goal

Publish explicit BIR semantic authority for local object identity,
array-decay/index/offset, element layout/range, and pointer-to-local-element
provenance when the producer has durable source facts.

## Core Rule

This is a producer-authority packet, not scalar load consumption. Do not infer
local-address provenance from raw shape, value names, testcase names, final
homes, or RV64 target fallback behavior.

## Read First

- ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
- ideas/closed/483_bir_semantic_local_memory_scalar_load_producer.md
- build/agent_state/483_step1_corrected_local_load_search/audit.md
- build/agent_state/483_step1_corrected_local_load_search/local_load_rows.tsv
- review/483_step1_local_load_route_review.md

## Current Target

- First blocked local-load representatives:
  - `src/pr22098-1.c`: compound literal array element through pointer
    `p#L1 = &<clit>#L3[(++a#L0)]`;
  - `src/pr38048-1.c`: local pointer `a = mat` with
    `det += a#L1[i#L3][0]`;
  - `src/multi-ix.c`: direct local-array element samples that need precise
    function/site separation from variadic/va_arg ownership.
- First packet:
  - audit which local-address/array-element authority shape is bounded enough
    for a producer contract.

## Non-Goals

- Scalar local-memory load consumption.
- Integer-pointer round-trip acceptance.
- Generic GEP/address lowering beyond selected local array-element authority.
- Local-memory store, direct-call metadata, memcpy/memset, alloca,
  function-signature, call-return, scalar-binop, or bootstrap repairs.
- RV64/MIR recovery or target lowering.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 483 proved the local-memory load bucket has no clean scalar local-object
representative without local-address/provenance prerequisites. This runbook
owns that prerequisite and must finish before returning to scalar local-load
production.

## Execution Rules

- Step 1 is audit/classification unless it identifies a bounded already-owned
  producer surface.
- Any implementation packet must publish durable authority or fail-closed
  status that later local-load producers can consume.
- Classification-only proof:

```sh
git diff --check
```

- Code-changing proof:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Local-Address Array-Element Authority Evidence

Inspect the blocked representative rows and determine which local-address or
array-element authority shape is bounded enough for a producer contract.
Completion means `todo.md` records accepted/rejected candidates and the exact
facts needed for source object, array decay, index/offset, layout/range, and
provenance.

### Step 2: Define Local-Address Provenance Contract

Define the smallest sound contract for local object address and array-element
provenance authority. Completion means the contract names accepted evidence,
required record fields, and fail-closed statuses for integer-pointer
round-trips, unknown provenance, global sources, aggregate/member ownership,
variadic/va_arg, runtime/call, F128, bootstrap, and raw-shape inference.

### Step 3: Implement Or Route Authority Producer

Implement the bounded producer packet if Step 2 identifies one. If current BIR
or HIR surfaces cannot provide the required facts, record the exact lower-level
owner and stop without changing scalar load consumption or RV64 lowering.

### Step 4: Residual Disposition And Close Readiness

Re-probe the selected representatives. Completion means lifecycle state records
whether local-load production can resume, another prerequisite remains first,
or this source idea is complete as a negative/blocked producer result.
