# Dynamic Local-Array Ordered Effect-Source Stream Builder

Status: Open
Type: BIR/prepared ordered effect-source stream builder idea
Parent: `ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md`
Source Evidence:
- `build/agent_state/497_step4_lower_blocker/blocker.md`
- `build/agent_state/497_step5_residual_disposition/disposition.md`
- `review/497_step4_bounded_effect_scan_review.md`
Owning Layer: Lower effect-source owner for dynamic local-array interval scans

## Goal

Build the production lower-owned ordered effect-source stream for the selected
dynamic local-array proof-source-to-endpoint interval so idea 497 can consume a
real builder-backed bounded scan instead of synthetic coverage inputs.

## Why This Exists

Idea 497 supplied endpoint bridge records and selected proof-source
coordinates, but Step 4 could not truthfully publish available no-clobber facts
because no production builder enumerates the relevant prepared/BIR effects in
one comparable ordered stream. Without this owner, idea 497 and the upstream
idea 494 must remain fail-closed.

## In Scope

- Consume `bir::LocalArrayEndpointBridgeRecord` rows and selected proof-edge
  path records for the same dynamic local-array producer.
- Establish one comparable ordered proof-source-to-endpoint interval without
  treating `lir_producer_instruction_index` as a prepared traversal index, BIR
  instruction index, or ordered effect endpoint.
- Populate ordered effect-source records from prepared/BIR sources for
  assignments/redefinitions, memory accesses, phi/alias transfers,
  calls/helpers, inline asm, publications, move bundles, parallel copies, and
  unknown modeled effects.
- Fail closed with precise statuses when interval boundaries cannot be ordered,
  an effect source lacks a comparable coordinate, or a required effect family
  is not modeled.
- Wire the bounded effect scan record so the interval classifier can consume
  builder-backed scan evidence without requiring the legacy
  `endpoint_bridge_available` compatibility boolean as a separate availability
  source.
- Add focused coverage for a clean no-clobber interval plus clobber, alias,
  unknown effect, missing coordinate, and boundary-ordering failures.

## Out Of Scope

- Rebuilding the Step 3 endpoint bridge already owned by idea 497.
- Publishing idea 494 available interval facts before this builder exists.
- Populating idea 489 proof facts, idea 486 checker inputs, idea 490
  certification, idea 484 packaging, scalar local-load consumption, or RV64/MIR
  lowering.
- Treating caller-supplied coverage booleans, synthetic effect vectors,
  selected-path availability, legacy bridge flags, testcase names, final homes,
  or target behavior as evidence of availability.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- A production builder populates comparable ordered effect-source records for
  the selected proof-source-to-endpoint interval from prepared/BIR data.
- The builder covers assignments/redefinitions, memory accesses,
  phi/alias transfers, calls/helpers, inline asm, publications, move bundles,
  parallel copies, and unknown modeled effects, or fails closed with a precise
  unavailable status for any unowned family.
- A bounded scan record can prove a clean interval available without depending
  on synthetic coverage booleans or the legacy `endpoint_bridge_available`
  compatibility boolean.
- Focused tests or probes cover a no-clobber representative and fail-closed
  representatives for clobber, alias, unknown effect, missing comparable
  coordinate, and boundary-ordering failures.
- Residual disposition states whether idea 497 can resume and consume the
  builder-backed scan.

## Reviewer Reject Signals

Reject patches that:

- infer no-clobber or effect coverage from caller-supplied booleans, synthetic
  effect vectors, selected-path availability, testcase shape, final homes,
  target behavior, or legacy bridge flags;
- treat `lir_producer_instruction_index` as a prepared traversal index, BIR
  instruction index, or ordered effect endpoint;
- publish available bounded-scan or interval facts without a production
  prepared/BIR effect-source builder owning the selected interval;
- skip assignments/redefinitions, memory accesses, calls/helpers, inline asm,
  publications, move bundles, parallel copies, phi/alias transfers, or unknown
  modeled effects when the selected interval can cross them;
- leave `evaluate_local_array_interval_effect` dependent on the legacy
  `endpoint_bridge_available` compatibility boolean when a valid
  builder-backed bounded scan record is present;
- populate downstream proof facts, checker inputs, packaging, scalar load
  consumption, or RV64/MIR lowering as proof of this lower builder;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as progress;
- retain the old synthetic coverage failure mode behind renamed helpers or
  status fields.
