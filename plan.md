# BIR Local Array Address Derivation And Index-Range Authority Carrier Plan

Status: Active
Source Idea: ideas/open/485_bir_local_array_address_derivation_index_range_authority_carrier.md
Activated From: ideas/closed/484_bir_semantic_local_address_provenance_array_element_authority.md

## Purpose

Create the lower-level authority carrier needed before local-address
provenance packaging or scalar local-load production can resume.

## Goal

Publish durable local-array address derivation and index-range authority facts
from producer-owned data, not route-local lowering coincidences.

## Core Rule

Do not infer authority from route-local `element_slots`, `base_index`, lowered
index values, type text, testcase names, value names, dump order, final homes,
or target fallback behavior without durable producer identity.

## Read First

- ideas/open/485_bir_local_array_address_derivation_index_range_authority_carrier.md
- ideas/closed/484_bir_semantic_local_address_provenance_array_element_authority.md
- build/agent_state/484_step3_local_address_authority_producer/blocker.md
- build/agent_state/484_step4_residual_disposition/disposition.md

## Current Target

- First accepted blocked shape:
  - `src/pr38048-1.c`
  - `decl mat: int[2][1]`
  - `decl a: int*[1] = mat#L0`
  - `det#L2 += a#L1[i#L3][0]`
- First packet:
  - audit existing BIR/HIR local-memory lowering surfaces and determine whether
    a durable source-object/derivation/index-range carrier can be produced.

## Non-Goals

- Scalar local-load consumption.
- Packaging into idea 484's higher-level provenance record.
- Generic GEP/address lowering beyond local array-element carrier work.
- Integer-pointer round-trip acceptance.
- Local-memory stores, calls, memcpy/memset, alloca, bootstrap, aggregate
  member, union/object-representation, F128, volatile/atomic, complex, vector,
  or RV64/MIR lowering.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 484 proved the higher-level local-address provenance record cannot be
implemented until local-array address derivation, ordered element paths,
dynamic index range proof, layout/range, and provenance statuses exist as
durable producer facts.

## Execution Rules

- Step 1 is audit/classification unless it identifies a bounded carrier surface
  already supported by current producer data.
- Any implementation packet must add durable authority or explicit unavailable
  status coverage.
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

### Step 1: Audit Local Array Carrier Inputs

Inspect local-memory lowering carriers and representative rows to identify
which source-object, derivation, element-path, index/range, layout/range, and
provenance facts are currently durable versus route-local. Completion means
`todo.md` records accepted carrier inputs and missing lower-level facts.

### Step 2: Define Address Derivation And Index-Range Carrier Contract

Define the durable carrier shape and unavailable statuses. Completion means the
contract names fields for source object identity, derivation site, ordered
element path, index identities, range proof, path/dominance validity,
layout/range, scalar in-bounds status, and provenance status.

### Step 3: Implement Or Route Carrier Producer

Implement the bounded carrier packet if Step 2 identifies one. If current data
cannot prove index ranges or derivation identity without raw-shape inference,
record the exact lower owner and stop without changing idea 484 packaging,
scalar loads, or RV64 lowering.

### Step 4: Residual Disposition And Close Readiness

Re-probe the selected representatives and decide whether idea 485 is complete,
blocked by another lower-level source, or ready to hand off to idea 484
packaging work.
