# Dynamic Local-Array Selected Proof-Edge Path Record Collector/Population Plan

Status: Active
Source Idea: ideas/open/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md
Activated From: ideas/closed/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md

## Purpose

Populate real selected proof-edge path records using the API surface completed
by idea 492.

## Goal

Produce explicit available/unavailable
`local_array_selected_proof_edge_paths` records from prepared branch/control
flow facts and matching dynamic local-array `lir_producer_lookup_key` rows.

## Core Rule

Do not let downstream proof population consume helper-local reachability,
dominance, or branch-shape facts directly. Collector output must publish
explicit records or explicit unavailable statuses keyed to the exact
`lir_producer_lookup_key`.

## Read First

- ideas/open/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md
- ideas/closed/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
- build/agent_state/492_step3_selected_proof_edge_record_api/summary.md
- build/agent_state/492_step4_residual_disposition/disposition.md

## Current Target

- Collector inputs:
  - `LocalArrayElementPathRecord` rows and `lir_producer_lookup_key`;
  - prepared branch/compare facts and selected successor labels;
  - reachability/path coverage helpers;
  - dominance or guard validity helpers.
- Collector outputs:
  - available selected proof-edge path records;
  - fail-closed unavailable records for missing/mismatched inputs.

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

Idea 492 supplied the storage, evaluator, and statuses. This runbook owns the
producer that fills those records from prepared branch/control-flow facts and
local-array path records. Display/printer exposure is optional and should not
displace the collector if semantic population can proceed.

## Execution Rules

- Step 1 is audit/classification unless the exact collector path is already
  bounded.
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

### Step 1: Audit Collector Inputs And Matching Keys

Inspect prepared branch/compare facts, successor labels,
reachability/dominance helpers, local-array path records, and
`lir_producer_lookup_key` matching. Completion means `todo.md` records whether
the collector can be bounded or names the exact lower blocker.

### Step 2: Define Collector Population Contract

Define source facts, matching rules, selected outcome, edge tuple, path
coverage, dominance/guard validity, unavailable statuses, and same-block
fail-closed behavior. Completion means the contract can drive focused tests.

### Step 3: Implement Or Route Collector Population

Implement the bounded collector if Step 2 identifies one. If current inputs
cannot truthfully populate records, record the exact lower owner and stop
without changing interval effects or downstream consumers.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected representatives and decide whether 493 is complete, blocked
by another lower-level source, or ready to hand forward to selected path
certification or interval effect/no-clobber classification.
