# Dynamic Local-Array LIR Producer Interval Effect Classifier Plan

Status: Active
Source Idea: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
Resumed After: ideas/closed/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md

## Purpose

Resume idea 494 after the lower endpoint/effect bridge work closed. The
classifier already has a fail-closed interval status surface; it can now publish
available same-value/no-clobber interval facts only by consuming the stored
ordered effect-source stream supplied through the endpoint bridge.

## Goal

Classify dynamic local-array index intervals from selected proof-source edge to
LIR producer address derivation as available or precisely unavailable, without
synthetic no-clobber inference.

## Core Rule

`available` may be emitted only when dynamic-index identity, selected path
binding, endpoint bridge authority, and exactly one matching production
`local_array_ordered_effect_source_streams` record prove the bounded interval is
same-value/no-clobber. Selected path availability, testcase shape, final homes,
target behavior, synthetic bridge flags, caller-supplied effect vectors, and
`lir_producer_instruction_index` alone remain insufficient.

## Read First

- ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
- build/agent_state/494_step2_interval_effect_contract/contract.md
- build/agent_state/494_step3_interval_effect_status_surface/summary.md
- build/agent_state/494_step4_residual_disposition/disposition.md
- ideas/closed/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
- build/agent_state/497_step5_residual_disposition_after_498/disposition.md
- ideas/closed/498_dynamic_local_array_ordered_effect_source_stream_builder.md

## Resumed State

- Completed 494 work:
  - Step 1 identified the missing endpoint/effect bridge blocker.
  - Step 2 defined the interval classifier contract and status vocabulary.
  - Step 3 implemented the fail-closed status surface.
  - Step 4 parked 494 until a lower endpoint/effect owner supplied the missing
    bridge and scan.
- Completed lower work:
  - Idea 497 closed after consuming the stored ordered effect-source stream
    through the endpoint bridge.
  - Idea 497 Step 5 says no lower endpoint/effect blocker remains for 494.

## Current Target

- Inputs:
  - populated selected proof-edge path records;
  - dynamic local-array producer rows keyed by `lir_producer_lookup_key`;
  - structured dynamic-index identity from local-array element paths and proof
    compare operands;
  - endpoint bridge records;
  - production-populated ordered effect-source stream records.
- Outputs:
  - available same-value/no-clobber interval facts for clean bounded rows;
  - precise unavailable statuses for missing producer keys, missing
    coordinates, unsupported roles, dynamic-index mismatch, missing path/order,
    missing endpoint bridge, selected-path-only evidence, duplicate streams,
    clobbers, alias/phi, unknown effects, unordered boundaries, and coordinate
    confusion;
  - residual disposition stating whether idea 490 can resume path/no-clobber
    certification.

## Non-Goals

- Re-implementing selected proof-edge path record collection.
- Reopening closed ideas 497 or 498 unless a concrete regression proves their
  handback wrong.
- Populating idea 489 proof facts directly.
- Populating idea 486 checker inputs directly.
- Idea 490 path/no-clobber certification itself.
- Idea 484 packaging, scalar local-load consumption, or RV64/MIR lowering.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Idea 494 owns interval effect classification facts. Closed idea 497 owns the
lower bridge from LIR producer coordinates to ordered prepared/BIR endpoint and
effect-stream evidence. This runbook resumes at the consumer boundary and must
turn the previously fail-closed status surface into truthful available facts
only for rows backed by clean stored stream evidence.

## Execution Rules

- Preserve `lir_producer_instruction_index` as a LIR producer-site coordinate;
  do not reinterpret it as prepared traversal order, BIR instruction index, or
  effect endpoint.
- Keep all existing fail-closed statuses distinguishable unless the source idea
  explicitly allows a narrower mapping.
- Positive availability must consume exactly one matching production ordered
  effect-source stream for the selected proof-source-to-endpoint interval.
- Empty records, duplicate streams, path-only evidence, unsupported statuses,
  unordered boundaries, missing coordinates, and coordinate confusion must fail
  closed.
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

### Step 5: Publish Available Interval Facts From Stored Stream

Wire the 494 interval classifier to the lower stored-stream consumer from
closed idea 497. A clean selected proof-edge path and dynamic local-array
producer row may classify as `available` only when the dynamic index matches,
the selected path covers the producer, the endpoint bridge matches the producer
key, and exactly one matching production ordered effect-source stream reports a
clean bounded interval.

Completion means focused backend coverage proves one production-backed
available representative and preserves fail-closed behavior for missing bridge,
selected-path-only inference, duplicate stream, clobber, alias/phi, unknown
effect, missing coordinate, unordered boundary, and coordinate-confusion
representatives.

### Step 6: Residual Disposition For Idea 490

Re-probe available and fail-closed representatives after Step 5 and decide
whether idea 490 path/no-clobber certification can resume, or whether another
lower interval/effect owner remains first.

Completion means `todo.md` records the residual disposition, names the next
idea that should resume, and identifies any remaining lower blocker if 490
cannot resume.
