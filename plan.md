# Dynamic Local-Array LIR Producer Endpoint Bridge Effect Scan Plan

Status: Active
Source Idea: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Activated From: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md

## Purpose

Provide the lower endpoint/effect prerequisite that blocked plan 494 after its
fail-closed interval status surface landed.

## Goal

Bridge dynamic local-array LIR producer rows to ordered prepared/BIR
address-derivation endpoints and scan the selected proof-source-to-endpoint
interval for real effects.

## Core Rule

Do not infer endpoint order or same-value/no-clobber from selected path
availability, testcase shape, final homes, target behavior, synthetic bridge
flags, or `lir_producer_instruction_index` alone. Endpoint and effect facts
must come from an authoritative prepared/BIR bridge plus a bounded effect scan,
or fail closed with a precise status.

## Read First

- ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
- ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
- build/agent_state/494_step3_interval_effect_status_surface/summary.md
- build/agent_state/494_step4_residual_disposition/disposition.md

## Current Target

- Inputs:
  - dynamic local-array `lir_producer_lookup_key` rows;
  - `lir_producer_instruction_index` as a LIR producer-site coordinate;
  - selected proof-source/path records from the existing 493/494 surfaces;
  - prepared/BIR address derivation and effect-stream surfaces.
- Outputs:
  - authoritative prepared/BIR address-derivation endpoint binding for the
    same local-array producer;
  - bounded proof-source-to-endpoint effect scan results;
  - precise unavailable statuses for missing endpoint, clobber, alias, unknown
    effect, and coordinate confusion.

## Non-Goals

- Populating idea 489 proof facts directly.
- Populating idea 486 checker inputs directly.
- Idea 490 path/no-clobber certification beyond this lower prerequisite.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Rewriting selected proof-edge path collection from idea 493.
- Treating selected-path availability, final homes, target behavior, testcase
  names, or synthetic bridge flags as proof.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Plan 494 now owns only a fail-closed interval status surface. This runbook owns
the lower bridge from LIR producer coordinates into ordered prepared/BIR
endpoint/effect evidence. If the bridge cannot be built truthfully, stop with
the exact lower blocker rather than publishing available no-clobber facts.

## Execution Rules

- Step 1 is audit/classification unless the exact endpoint bridge is already
  bounded.
- Implementation packets must publish endpoint/effect evidence or precise
  unavailable statuses only; they must not populate downstream proof facts,
  checker inputs, packaging, scalar loads, or RV64 lowering.
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

### Step 1: Audit Producer-To-Endpoint Surfaces

Inspect local-array producer rows, prepared/BIR address-derivation records,
effect-stream ordering, and selected proof-source/path inputs. Completion
means `todo.md` records whether a bounded endpoint bridge can be built or names
the exact lower blocker.

### Step 2: Define Endpoint Bridge And Effect Scan Contract

Define the producer key, endpoint identity, interval boundary semantics,
effect classes, no-clobber criterion, and fail-closed statuses. Completion
means the contract can drive focused bridge and effect-scan tests without
coordinate confusion.

### Step 3: Implement Or Route Endpoint Bridge

Implement the bounded producer-key-to-prepared/BIR endpoint bridge if Step 2
identifies one. If current inputs cannot truthfully bind the endpoint, record
the exact lower owner and stop without publishing available interval facts.

### Step 4: Implement Or Route Bounded Effect Scan

Implement the selected proof-source-to-endpoint effect scan if the bridge is
available. Cover assignments/redefinitions, phi/alias transfers, calls/helpers,
inline asm, publications, move bundles, parallel copies, and unknown modeled
effects. If the scan cannot be built truthfully, record the exact lower owner
and stop.

### Step 5: Residual Disposition And Handback Readiness

Re-probe bridged and fail-closed representatives and decide whether idea 494
can resume available interval fact publication, or whether another lower
endpoint/effect owner remains first.
