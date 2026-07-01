# Dynamic Local-Array LIR Producer Endpoint Bridge Effect Scan Plan

Status: Active
Source Idea: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Activated From: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
Resumed After: ideas/closed/498_dynamic_local_array_ordered_effect_source_stream_builder.md

## Purpose

Finish the lower endpoint/effect prerequisite that blocked plan 494 by
consuming the production ordered effect-source stream built by closed idea 498.

## Goal

Bridge dynamic local-array LIR producer rows to ordered prepared/BIR
address-derivation endpoints and scan the selected proof-source-to-endpoint
interval for real effects, without synthetic availability.

## Core Rule

Do not infer endpoint order or same-value/no-clobber from selected path
availability, testcase shape, final homes, target behavior, synthetic bridge
flags, caller-supplied coverage booleans, caller-supplied effect vectors, or
`lir_producer_instruction_index` alone. Endpoint and effect facts must come
from the authoritative endpoint bridge plus the stored ordered effect-source
stream, or fail closed with a precise status.

## Read First

- ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
- review/497_step4_bounded_effect_scan_review.md
- build/agent_state/498_step5_residual_disposition/disposition.md
- ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
- build/agent_state/494_step3_interval_effect_status_surface/summary.md
- build/agent_state/494_step4_residual_disposition/disposition.md

## Resumed State

- Completed lower work:
  - Step 3 endpoint bridge records exist through
    `bir::LocalArrayEndpointBridgeRecord`.
  - Idea 498 closed after adding production-populated
    `local_array_ordered_effect_source_streams`.
  - The 498 handback says clean, clobber, alias/phi, unknown effect,
    missing-coordinate, and boundary-ordering representatives no longer have a
    lower blocker in idea 498.
- Blocking review context:
  - The previous Step 4 route was rejected because synthetic scan inputs could
    publish availability without real prepared/BIR effect-source population.
  - The interval classifier must not require the legacy
    `endpoint_bridge_available` compatibility flag when a valid stored stream
    record is available.

## Current Target

- Inputs:
  - dynamic local-array `lir_producer_lookup_key` rows;
  - `lir_producer_instruction_index` as a LIR producer-site coordinate only;
  - selected proof-source/path records from the existing 493/494 surfaces;
  - `LocalArrayEndpointBridgeRecord`;
  - production-populated ordered effect-source stream records from idea 498.
- Outputs:
  - truthful bounded proof-source-to-endpoint effect scan results;
  - precise unavailable statuses for missing endpoint, clobber, alias/phi,
    unknown effect, missing coordinate, unordered boundary, duplicate stream,
    path-only evidence, and coordinate confusion;
  - residual disposition that says whether idea 494 can resume available
    interval fact publication.

## Non-Goals

- Populating idea 489 proof facts directly.
- Populating idea 486 checker inputs directly.
- Idea 490 path/no-clobber certification beyond this lower prerequisite.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Rewriting selected proof-edge path collection from idea 493.
- Reopening closed idea 498 unless a concrete regression proves its handback
  wrong.
- Treating selected-path availability, final homes, target behavior, testcase
  names, synthetic bridge flags, synthetic effect-source lists, or synthetic
  coverage booleans as proof.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Plan 494 owns the eventual interval status consumer. Plan 497 owns the lower
bridge from LIR producer coordinates into ordered prepared/BIR endpoint/effect
evidence. Closed idea 498 now owns production population of ordered
effect-source streams. This runbook resumes 497 at the consumption and
handback boundary.

## Execution Rules

- Implementation packets must publish endpoint/effect evidence or precise
  unavailable statuses only; they must not populate downstream proof facts,
  checker inputs, packaging, scalar loads, or RV64 lowering.
- A positive available result must consume exactly one matching stored ordered
  effect-source stream for the bridged local-array producer interval.
- Empty records, duplicate matching records, path-only evidence, unsupported
  statuses, unordered boundaries, and missing coordinates must fail closed.
- Code-changing proof:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

- Lifecycle-only proof:

```sh
git diff --check
```

## Steps

### Step 4: Consume Stored Ordered Effect Stream

Replace the rejected synthetic bounded-scan route with consumption of
production-populated `local_array_ordered_effect_source_streams` through the
Step 3 endpoint bridge. The interval classifier must consume a valid stored
stream record without also requiring the legacy `endpoint_bridge_available`
compatibility boolean.

Completion means focused tests prove a clean production-populated stream can
classify as available, while empty records, path-only evidence, duplicate
matching records, missing endpoint, clobber, alias/phi, unknown effect,
missing-coordinate, unordered-boundary, and coordinate-confusion
representatives fail closed with distinguishable statuses.

### Step 5: Residual Disposition And Handback Readiness

Re-probe bridged and fail-closed representatives after Step 4 and decide
whether idea 494 can resume available interval fact publication, or whether
another lower endpoint/effect owner remains first.

Completion means `todo.md` records the residual disposition, names the next
idea that should resume, and identifies any remaining lower blocker if 494
cannot resume.
