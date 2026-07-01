# BIR Semantic Local-Memory Scalar Load Producer Plan

Status: Active
Source Idea: ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
Activated From: ideas/closed/422_bir_semantic_producer_high_impact_cleanup.md

## Purpose

Implement the first selected BIR semantic producer follow-up from idea 422:
ordinary scalar local-memory load facts.

## Goal

Publish explicit semantic producer facts for scalar local-memory loads only
when the producer has durable source object, access range, layout, and result
identity evidence.

## Core Rule

Keep this a BIR semantic producer repair. Do not recover missing facts in
prepared/RV64 lowering from raw shape, value names, testcase names, final homes,
or target fallback inference.

## Read First

- ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
- ideas/closed/422_bir_semantic_producer_high_impact_cleanup.md
- build/agent_state/422_step2_bir_producer_buckets/bucket_table.tsv
- build/agent_state/422_step3_first_producer_packet/decision.md
- build/agent_state/422_step4_residual_disposition/disposition.md

## Current Target

- Selected bucket: local-memory load, 79 rows.
- Representative rows:
  - `src/20041124-1.c`
  - `src/20071219-1.c`
  - `src/991228-1.c`
  - `src/multi-ix.c`
  - `src/pr22098-1.c`
- First packet:
  - audit focused local-memory load probes and select the narrowest scalar
    local-load shape for contract definition.

## Non-Goals

- Local-memory GEP/address, store, direct-call metadata, memcpy/memset,
  alloca-derived, scalar/local-memory mixed, function-signature, call-return,
  scalar-binop, or bootstrap producer repair.
- Aggregate/member, pointer/provenance, byval/va_arg, volatile/atomic, complex,
  vector, or F128 load support unless a later lifecycle split selects them.
- RV64 target lowering, prepared consumer inference, branch/select consumers,
  stack-home materialization, call lowering, or return lowering.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 422 selected local-memory load as the largest coherent BIR semantic
producer bucket. This runbook starts with focused probes because the row-level
scan gives family labels and representatives, but not a complete implementation
contract for every local-load shape.

## Execution Rules

- Step 1 is probe/classification work; do not implement until a scalar
  local-load shape is selected.
- Any code-changing packet must include focused positive and fail-closed
  backend/BIR coverage.
- Keep proof local first, then use backend subset proof for code-changing
  packets.
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

### Step 1: Audit Focused Local-Load Representative Evidence

Inspect the selected representative rows and collect focused BIR/prepared
evidence for failing function, load instruction identity, source object or
frame slot, access range, scalar result type, and adjacent contaminating
features. Completion means `todo.md` records accepted and rejected first-packet
load shapes.

### Step 2: Define Scalar Local-Load Producer Contract

Define the smallest sound contract for admitting a scalar local-memory load
semantic producer. Completion means the contract names required explicit facts,
accepted ordinary C shape, and fail-closed statuses for GEP, store,
aggregate/member, pointer/provenance, byval/va_arg, volatile/atomic, complex,
vector, F128, bootstrap, and raw-shape inference.

### Step 3: Implement Or Route Scalar Local-Load Producer

Implement the bounded producer packet if Step 2 identifies one. If current BIR
or prepared surfaces cannot expose the required facts, record the exact
lower-level owner and stop without changing RV64 lowering or expectations.

### Step 4: Residual Disposition And Close Readiness

Re-probe the selected local-load representatives. Completion means lifecycle
state records whether the scalar local-load producer slice is complete, whether
another local-load subfamily remains, or whether work should hand off to the
next BIR producer bucket.
