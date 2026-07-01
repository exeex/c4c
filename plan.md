# Dynamic Local-Array Ordered Effect-Source Stream Builder Plan

Status: Active
Source Idea: ideas/open/498_dynamic_local_array_ordered_effect_source_stream_builder.md
Activated From: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md

## Purpose

Provide the lower ordered effect-source stream builder that blocked idea 497
and keeps idea 494 in fail-closed interval status mode.

## Goal

Populate production prepared/BIR effect-source records for the selected
dynamic local-array proof-source-to-endpoint interval so a bounded scan can
prove availability from real ordered evidence.

## Core Rule

Do not infer no-clobber, effect coverage, or endpoint ordering from synthetic
coverage booleans, caller-supplied effect vectors, selected-path availability,
legacy bridge flags, final homes, target behavior, testcase shape, or
`lir_producer_instruction_index` alone. Availability must come from a
production builder over prepared/BIR effect sources, or fail closed with a
precise status.

## Read First

- ideas/open/498_dynamic_local_array_ordered_effect_source_stream_builder.md
- ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
- build/agent_state/497_step4_lower_blocker/blocker.md
- build/agent_state/497_step5_residual_disposition/disposition.md
- review/497_step4_bounded_effect_scan_review.md

## Current Target

- Inputs:
  - `bir::LocalArrayEndpointBridgeRecord` rows from the Step 3 endpoint bridge;
  - selected proof-edge path records carrying proof-source coordinates;
  - prepared/BIR records that can expose assignments/redefinitions, memory
    accesses, phi/alias transfers, calls/helpers, inline asm, publications,
    move bundles, parallel copies, and unknown modeled effects.
- Outputs:
  - comparable ordered effect-source records for the selected
    proof-source-to-endpoint interval;
  - fail-closed statuses for missing coordinates, unsupported ordering,
    unmodeled effect families, clobber, alias, and unknown effect;
  - bounded scan records that idea 497 can consume without synthetic
    availability inputs.

## Non-Goals

- Rebuilding the Step 3 endpoint bridge.
- Publishing idea 494 available interval facts before the builder-backed scan
  exists.
- Populating idea 489 proof facts, idea 486 checker inputs, idea 490
  certification, idea 484 packaging, scalar local-load consumption, or RV64/MIR
  lowering.
- Rewriting selected proof-edge path collection from idea 493.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Idea 497 is parked because endpoint records and proof-source coordinates exist,
but no production ordered effect-source stream builder exists. This runbook
owns that lower builder. The builder must derive effect-source coverage from
prepared/BIR sources and make every effect source comparable against the same
proof-source-to-endpoint interval.

## Execution Rules

- Step 1 is audit/classification unless the exact prepared/BIR effect-source
  surfaces are already bounded.
- Implementation packets must build or route production ordered effect-source
  population; do not accept externally supplied coverage as progress.
- The classifier handoff must consume a valid builder-backed bounded scan
  record without separately requiring the legacy `endpoint_bridge_available`
  compatibility boolean.
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

### Step 1: Audit Prepared/BIR Effect-Source Surfaces

Inspect where assignments/redefinitions, memory accesses, phi/alias transfers,
calls/helpers, inline asm, publications, move bundles, parallel copies, and
unknown modeled effects are represented with comparable ordering. Completion
means `todo.md` records the exact builder inputs or the precise lower blocker.

### Step 2: Define Ordered Effect-Source Contract

Define effect-source identity, coordinate ordering, interval boundary
semantics, required effect families, clobber/alias/unknown classifications, and
fail-closed statuses. Completion means focused tests can distinguish real
builder-backed availability from missing or unmodeled effect-source coverage.

### Step 3: Implement Ordered Effect-Source Population

Add the production builder that consumes endpoint bridge records, selected
proof-edge records, and prepared/BIR effect sources to populate one comparable
ordered stream for the selected interval. Completion means the builder records
all in-scope effect families or fails closed with precise unavailable statuses.

### Step 4: Wire Bounded Scan Consumption

Route builder-backed ordered effect-source records into the bounded scan record
and interval classifier. Remove any requirement that a valid builder-backed
bounded scan also needs the legacy `endpoint_bridge_available` compatibility
boolean to publish its result. Completion means synthetic coverage alone cannot
produce availability, while a real clean builder-backed interval can.

### Step 5: Residual Disposition And 497 Handback

Re-probe clean, clobber, alias, unknown effect, missing-coordinate, and
boundary-ordering representatives. Decide whether idea 497 can resume and
consume the builder-backed bounded scan, or whether another lower owner remains
first.
