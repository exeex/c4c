# Dynamic Local-Array LIR Producer Interval Effect Classifier Plan

Status: Active
Source Idea: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
Activated From: ideas/closed/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md

## Purpose

Classify dynamic-index same-value/no-clobber interval effects now that selected
proof-edge path records are populated.

## Goal

Publish explicit available/unavailable interval effect facts keyed to populated
selected proof-edge path records and dynamic local-array `lir_producer_lookup_key`
rows.

## Core Rule

Do not infer same-value/no-clobber from selected path availability alone,
testcase shape, final homes, or target behavior. Interval facts must come from
explicit producer/prepared evidence or fail closed with a precise status.

## Read First

- ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
- ideas/closed/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md
- build/agent_state/493_step3_collector_population/summary.md
- build/agent_state/493_step4_residual_disposition/disposition.md

## Current Target

- Inputs:
  - populated `local_array_selected_proof_edge_paths` records;
  - local-array element path dynamic-index identity;
  - proof branch/compare identity and selected outcome;
  - `lir_producer_lookup_key` binding;
  - prepared/BIR effect surfaces for assignments, phi/alias, calls/helpers,
    inline asm, publications, move bundles, and parallel copies.
- Outputs:
  - available same-value/no-clobber interval facts;
  - fail-closed unavailable records/statuses for unknown or clobbering effects.

## Non-Goals

- Re-implementing selected proof-edge path record API or collector population.
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

Idea 493 provides the selected branch/edge/path and dominance/guard side of the
proof. This runbook owns the separate interval classifier that proves the
dynamic index remains same-value and unclobbered across that selected interval.

## Execution Rules

- Step 1 is audit/classification unless the exact interval classifier path is
  already bounded.
- Any implementation packet must publish interval facts/statuses only; it must
  not populate proof facts, checker inputs, packaging, scalar loads, or RV64
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

### Step 1: Audit Interval Effect Inputs

Inspect dynamic-index identity, selected proof-edge records, interval boundary
surfaces, and effect/no-clobber inputs for assignments, phi/alias,
calls/helpers, inline asm, publications, move bundles, and parallel copies.
Completion means `todo.md` records whether a bounded classifier can be built or
names the exact lower blocker.

### Step 2: Define Interval Effect Classifier Contract

Define the interval key, start/end semantics, same-value criterion, effect
classes, unavailable statuses, and same-block fail-closed policy. Completion
means the contract can drive focused available/fail-closed tests.

### Step 3: Implement Or Route Interval Effect Classifier

Implement the bounded classifier if Step 2 identifies one. If current inputs
cannot truthfully classify interval effects, record the exact lower owner and
stop without changing proof population or downstream consumers.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected representatives and decide whether 494 is complete, blocked
by another lower-level source, or ready to hand forward to idea 490
path/no-clobber certification.
