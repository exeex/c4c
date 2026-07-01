# Dynamic Local-Array Selected Proof-Edge Path Certificate Plan

Status: Active
Source Idea: ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md
Activated From: ideas/closed/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md

## Purpose

Produce the lower selected proof-edge path certificate needed before dynamic
local-array interval effect/no-clobber work can resume.

## Goal

Publish durable selected proof edge/outcome, path coverage, and
dominance/guard validity facts keyed to `lir_producer_*` coordinates.

## Core Rule

Do not infer selected proof edges, path coverage, dominance, guard validity, or
same-block ordering from branch proximity, loop shape, value names, testcase
names, dump order, final homes, or target behavior. The certificate must be
explicit and keyed to the LIR producer site.

## Read First

- ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md
- ideas/closed/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
- build/agent_state/490_step3_certificate_producer_route/route.md
- build/agent_state/490_step4_residual_disposition/disposition.md

## Current Target

- Missing certificate fields:
  - proof branch/compare identity;
  - selected proof edge/outcome;
  - proof-source-to-LIR-producer path coverage;
  - dominance or guard validity under the selected outcome;
  - same-block ordering safety or explicit fail-closed status.
- First packet:
  - audit current prepared/BIR selected-edge and path surfaces for a bounded
    certificate producer.

## Non-Goals

- Dynamic-index same-value/no-clobber interval effect classification.
- Populating idea 489 proof facts or idea 486 checker inputs.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Prepared traversal/BIR instruction coordinate conversion.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 490 found candidate branch/compare proof sources and LIR producer binding
keys, but no durable selected-edge/path/order certificate. This runbook owns
that lower certificate only. Later interval effect/no-clobber classification
stays separate.

## Execution Rules

- Step 1 is audit/classification unless an already-bounded selected proof-edge
  certificate surface exists.
- Any implementation packet must publish explicit certificate facts or
  unavailable statuses and must not mark proof-population or checker inputs
  available directly.
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

### Step 1: Audit Selected Proof-Edge Path Certificate Inputs

Inspect prepared branch/compare facts, selected edge/outcome surfaces,
dominance/reachability helpers, `lir_producer_*` keys, and same-block ordering
boundaries. Completion means `todo.md` records available certificate inputs and
the exact missing lower facts, if any.

### Step 2: Define Selected Proof-Edge Path Certificate Contract

Define the selected-edge path certificate shape and unavailable statuses.
Completion means the contract names proof-source identity, selected
edge/outcome, LIR producer key, path coverage, dominance/guard validity,
same-block ordering policy, and coordinate-confusion rejections.

### Step 3: Implement Or Route Selected Edge Path Certificate

Implement the bounded certificate packet if Step 2 identifies one. If current
data cannot prove selected edge/path coverage without inference, record the
exact lower owner and stop without changing interval effects, proof population,
checker inputs, packaging, scalar loads, or RV64 lowering.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected representatives and decide whether 491 is complete, blocked
by another lower-level source, or ready to hand forward to dynamic-index
interval effect/no-clobber classification.
