# BIR Dynamic Local-Array Consumer Coordinate Prepared Exposure Plan

Status: Active
Source Idea: ideas/open/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md
Activated From: ideas/closed/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md

## Purpose

Expose dynamic local-array consumer coordinates so proof-source/path/no-clobber
population can later bind branch facts to the exact consumer interval.

## Goal

Publish durable consumer-coordinate records and prepared lookup keys for
supported dynamic local-array element paths.

## Core Rule

This is coordinate/prepared-exposure metadata only. Do not infer coordinates
from dump order, names, branch proximity, loop shape, final homes, or target
behavior, and do not populate proof-source/path/no-clobber facts here.

## Read First

- ideas/open/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md
- ideas/closed/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md
- build/agent_state/487_step3_route_consumer_coordinate_prerequisite/route.md
- build/agent_state/487_step4_residual_disposition/disposition.md

## Current Target

- Missing prerequisite:
  - dynamic local-array element-path consumer coordinates
  - prepared lookup key shared with prepared control-flow branch-condition
    records
- First packet:
  - audit BIR/prepared surfaces that can carry function, block, instruction,
    path result, source object, derivation result, dynamic index, and consumer
    role fields.

## Non-Goals

- Proof-source population.
- Path/dominance or no-clobber facts.
- Marking dynamic range proofs available.
- Idea 486 checker vocabulary changes.
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

Idea 487 proved proof population cannot proceed until dynamic local-array
element paths are exposed with stable consumer coordinates and lookup keys. This
plan owns that exposure only.

## Execution Rules

- Step 1 is audit/classification unless a bounded coordinate surface already
  exists.
- Any implementation packet must publish durable coordinates or fail-closed
  statuses without changing proof availability.
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

### Step 1: Audit Consumer Coordinate Exposure Inputs

Inspect BIR local-array element-path records, prepared traversal, and prepared
control-flow lookup surfaces for durable consumer coordinate fields. Completion
means `todo.md` records available fields, missing coordinates, and whether a
bounded exposure contract exists.

### Step 2: Define Consumer Coordinate Exposure Contract

Define required coordinate fields, lookup key, operation roles, and fail-closed
statuses. Completion means the contract excludes proof-source/path/no-clobber
population and raw-shape/name/proximity inference.

### Step 3: Implement Or Route Prepared Exposure

Implement the bounded coordinate/prepared-exposure packet if Step 2 identifies
one. If current traversal data cannot provide durable coordinates without
inference, record the exact lower owner and stop without changing proof
population, packaging, scalar loads, or RV64 lowering.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected dynamic local-array paths and decide whether 488 is complete,
blocked by another lower-level source, or ready to hand back to proof
population.
