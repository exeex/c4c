# BIR Index Range Proof And Path-Dominance Carrier Plan

Status: Active
Source Idea: ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
Activated From: ideas/closed/485_bir_local_array_address_derivation_index_range_authority_carrier.md

## Purpose

Provide the dynamic index range proof and path/dominance carrier needed before
dynamic local-array GEP rows can become available.

## Goal

Publish durable range proof, path/dominance validity, and index no-clobber facts
for dynamic local-array indices.

## Core Rule

Do not infer index range from loop shape, value names, testcase names, dump
order, final homes, or RV64 target behavior. A dynamic row is available only
with explicit proof source, path coverage, dominance, and no-clobber facts.

## Read First

- ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
- ideas/closed/485_bir_local_array_address_derivation_index_range_authority_carrier.md
- build/agent_state/485_step3_local_array_carrier_producer/summary.md
- build/agent_state/485_step4_residual_disposition/disposition.md

## Current Target

- First blocked dynamic carrier row:
  - dynamic local-array GEP currently unavailable as
    `missing_index_range_proof`
  - representative family: `pr38048-1.c` style `a#L1[i#L3][0]`
- First packet:
  - audit current control-flow/proof surfaces for explicit range proof,
    path/dominance validity, and index no-clobber facts.

## Non-Goals

- Local-array source-object, derivation, or layout carrier records already
  completed by idea 485.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Broad generic range analysis beyond dynamic local-array index proof.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 485 publishes durable source-object, derivation, and constant
element-path/layout carrier records. Dynamic element paths remain unavailable
until an index proof carrier can show that the index is in range at the
consumer access and unchanged along the covered path.

## Execution Rules

- Step 1 is audit/classification unless it identifies an existing bounded
  proof surface.
- Any implementation packet must add durable proof records or explicit
  unavailable statuses.
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

### Step 1: Audit Index Range Proof Inputs

Inspect current BIR/HIR/control-flow surfaces for dynamic index range proof,
path/dominance, and no-clobber evidence. Completion means `todo.md` records
available proof inputs, missing facts, and whether a bounded contract exists.

### Step 2: Define Range Proof And Path-Dominance Contract

Define required carrier fields and unavailable statuses. Completion means the
contract names proof source, index identity, bounds, comparison predicate,
path/dominance validity, no-clobber validity, and fail-closed statuses.

### Step 3: Implement Or Route Proof Carrier

Implement the bounded proof carrier if Step 2 identifies one. If current data
cannot prove path/dominance or no-clobber without raw-shape inference, record
the exact lower owner and stop without changing idea 485, idea 484, scalar
loads, or RV64 lowering.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected dynamic index representatives and decide whether 486 is
complete, blocked by another lower-level source, or ready to hand back to idea
485/484 packaging work.
