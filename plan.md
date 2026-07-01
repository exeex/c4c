# BIR Dynamic Local-Array Proof Population From LIR Coordinates Plan

Status: Active
Source Idea: ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
Activated From: ideas/closed/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md

## Purpose

Resume dynamic local-array proof population now that the LIR producer-coordinate
surface exists.

## Goal

Populate real proof-source, path/dominance, and no-clobber facts using
`lir_producer_*` coordinates and feed them into the idea 486 checker.

## Core Rule

Bind proof facts through the committed LIR producer-site coordinate and
`lir-producer:` key. Do not infer from loop shape, names, branch proximity,
dump order, final homes, or target behavior, and do not treat LIR instruction
indices as prepared traversal/BIR instruction indices.

## Read First

- ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
- ideas/closed/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md
- build/agent_state/488_step3_consumer_coordinate_exposure/summary.md
- build/agent_state/488_step4_residual_disposition/disposition.md
- ideas/closed/486_bir_index_range_proof_path_dominance_carrier.md

## Current Target

- Available prerequisite:
  - `LocalArrayElementPathRecord::lir_producer_*`
  - `lir-producer:` stable lookup keys
- Still missing:
  - proof-source records for lower/upper bound branch or compare facts
  - path/dominance coverage from proof source to LIR producer site
  - index no-clobber/same-value interval facts
- First packet:
  - audit whether existing prepared branch/compare and control-flow surfaces can
    bind to the LIR producer coordinate without raw-shape inference.

## Non-Goals

- Redefining idea 486 checker/status vocabulary.
- Extending idea 488 coordinate exposure.
- Prepared traversal/BIR instruction coordinate conversion.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 488 provides truthful LIR producer-site coordinates for address-derivation
paths. This runbook owns only the next proof-population packet using those
coordinates.

## Execution Rules

- Step 1 is audit/classification unless a bounded proof population packet is
  already evident.
- Any implementation packet must feed explicit facts into the existing idea 486
  checker and preserve fail-closed statuses.
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

### Step 1: Audit LIR Coordinate Proof Population Inputs

Inspect prepared branch/compare facts, control-flow/path surfaces, no-clobber
evidence, and dynamic local-array path `lir_producer_*` fields. Completion
means `todo.md` records whether proof population can bind through LIR producer
coordinates or names the exact missing lower owner.

### Step 2: Define LIR Coordinate Proof Population Contract

Define accepted proof-source, path/dominance, and no-clobber facts keyed by
LIR producer coordinates. Completion means fail-closed statuses reject
raw-shape/name/proximity inference, prepared/BIR coordinate confusion, and
missing proof facts.

### Step 3: Implement Or Route Proof Population

Implement the bounded proof population packet if Step 2 identifies one. If
current surfaces cannot prove path/dominance or no-clobber from LIR coordinates
without inference, record the exact lower owner and stop without changing idea
484, scalar loads, or RV64 lowering.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected dynamic representatives and decide whether 489 is complete,
blocked by another lower-level source, or ready to hand back to idea 484
packaging work.
