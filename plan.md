# BIR Dynamic Local-Array Proof-Source Path/No-Clobber Population Plan

Status: Active
Source Idea: ideas/open/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md
Activated From: ideas/closed/486_bir_index_range_proof_path_dominance_carrier.md

## Purpose

Populate the real proof inputs needed by idea 486's dynamic local-array range
checker.

## Goal

Produce real proof-source, path/dominance, and index no-clobber facts for
supported dynamic local-array indices.

## Core Rule

Do not infer range proof from loop shape, variable names, testcase names, dump
order, final homes, or RV64 target behavior. Dynamic rows become available only
when real proof-source, path/dominance, and no-clobber facts feed the checker.

## Read First

- ideas/open/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md
- ideas/closed/486_bir_index_range_proof_path_dominance_carrier.md
- build/agent_state/486_step3_range_proof_path_dominance_carrier/summary.md
- build/agent_state/486_step4_residual_disposition/disposition.md

## Current Target

- First blocked dynamic carrier state:
  - real dynamic local-array rows remain `missing_index_range_proof`
  - proof-source/path/no-clobber records are not populated from real
    control-flow facts yet
- First packet:
  - audit current control-flow branch/compare surfaces and no-clobber evidence
    for a bounded real population packet.

## Non-Goals

- Redefining idea 486's checker/status carrier.
- Local-array source-object, derivation, or layout carrier records completed by
  idea 485.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Broad generic range analysis beyond dynamic local-array proof population.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 486 provides the status/checker surface. This plan owns only the missing
real inputs: proof source, branch/path coverage, dominance, and index
same-value/no-clobber evidence.

## Execution Rules

- Step 1 is audit/classification unless it identifies a bounded real proof
  population surface.
- Any implementation packet must feed idea 486's checker with real populated
  facts and preserve fail-closed statuses.
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

### Step 1: Audit Real Proof Population Inputs

Inspect control-flow branch/compare facts, local-array dynamic index records,
and same-value/no-clobber evidence. Completion means `todo.md` records whether
real proof-source/path/no-clobber population is bounded or names the exact
lower-level blocker.

### Step 2: Define Real Proof Population Contract

Define how real proof-source, path/dominance, and no-clobber facts feed the
idea 486 checker. Completion means the contract names accepted proof shapes,
required identities, path coverage, no-clobber rules, and fail-closed statuses.

### Step 3: Implement Or Route Real Proof Population

Implement the bounded population packet if Step 2 identifies one. If current
control-flow data cannot prove path/dominance or no-clobber without raw-shape
inference, record the exact lower owner and stop without changing idea 484,
scalar loads, or RV64 lowering.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected dynamic representatives and decide whether 487 is complete,
blocked by another lower-level source, or ready to hand back to idea 485/484
packaging work.
