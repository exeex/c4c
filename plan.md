# Dynamic Local-Array Selected Proof-Edge Path Record/Status API Plan

Status: Active
Source Idea: ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
Activated From: ideas/closed/491_dynamic_local_array_selected_proof_edge_path_certificate.md

## Purpose

Create the prepared record/status API needed before selected proof-edge path
certification can publish durable path facts.

## Goal

Publish explicit selected proof-edge path records keyed to
`lir_producer_lookup_key`, or route to the next lower API blocker without
inferring from raw branch shape.

## Core Rule

Do not let downstream proof population, checker inputs, or target behavior
consume helper-local reachability/dominance calculations directly. Selected
proof-edge path facts must be represented as explicit prepared records or
explicit unavailable statuses.

## Read First

- ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
- ideas/closed/491_dynamic_local_array_selected_proof_edge_path_certificate.md
- build/agent_state/491_step3_selected_edge_path_route/route.md
- build/agent_state/491_step4_residual_disposition/disposition.md

## Current Target

- Missing surface:
  - selected proof-edge path record/status API keyed to
    `lir_producer_lookup_key`.
- Required record fields:
  - all `lir_producer_*` fields;
  - proof branch/compare identity;
  - dynamic-index operand role and bound contribution when available;
  - selected outcome and edge tuple;
  - path coverage and dominance/guard validity;
  - fail-closed status vocabulary.
- First packet:
  - audit current prepared record homes and helper inputs for a bounded API
    packet.

## Non-Goals

- Dynamic-index same-value/no-clobber interval effect classification.
- Populating idea 489 proof facts or idea 486 checker inputs.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- LIR-to-prepared/BIR same-block ordering conversion beyond unavailable status
  reporting.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 491 found that raw `PreparedBranchCondition` labels and helper-level
reachability/dominance logic are not durable proof facts. This runbook owns the
record/status API that can publish those facts explicitly for each dynamic
local-array LIR producer key.

## Execution Rules

- Step 1 is audit/classification unless the exact record/status home is already
  obvious and bounded.
- Any implementation packet must publish records/statuses only; it must not
  populate proof facts, checker inputs, packaging, scalar loads, or RV64
  lowering.
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

### Step 1: Audit Selected Proof-Edge Record Homes

Inspect prepared record/status surfaces, selected local-array path records,
prepared branch/compare facts, and helper inputs for the narrow record/status
API. Completion means `todo.md` records the exact candidate home or the next
lower blocker.

### Step 2: Define Record/Status API Contract

Define the record fields, status enum/vocabulary, dump keys, and fail-closed
behavior. Completion means the contract names every field from the 491 route
disposition and the boundaries for same-block ordering.

### Step 3: Implement Or Route Record/Status API

Implement the bounded prepared record/status API if Step 2 identifies one. If
current ownership cannot publish it without another lower surface, record the
exact owner and stop without changing interval effects or downstream consumers.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected representatives and decide whether 492 is complete, blocked
by another lower-level source, or ready to hand forward to selected proof-edge
path certification.
