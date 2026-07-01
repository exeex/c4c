# Dynamic Local-Array LIR Producer Endpoint Bridge Effect Scan

Status: Open
Type: BIR/prepared endpoint bridge and bounded effect scan idea
Parent: `ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md`
Source Evidence:
- `build/agent_state/494_step3_interval_effect_status_surface/summary.md`
- `build/agent_state/494_step4_residual_disposition/disposition.md`
Owning Layer: Lower endpoint/effect owner for local-array address derivation
Lifecycle Note: Parked after Step 5. Step 3 supplied bounded endpoint bridge
records, but 497 cannot complete and 494 cannot resume availability until
`ideas/open/498_dynamic_local_array_ordered_effect_source_stream_builder.md`
builds production ordered effect-source stream population for the selected
proof-source-to-endpoint interval.

## Goal

Provide the lower-owned bridge from a dynamic local-array
`lir_producer_lookup_key` / `lir_producer_instruction_index` row to the ordered
prepared/BIR address-derivation endpoint for the same local-array producer, and
scan the selected proof-source-to-endpoint interval for real effects.

## Why This Exists

Idea 494 can publish precise fail-closed interval statuses, but it cannot
truthfully publish available same-value/no-clobber facts from selected path
records plus LIR producer coordinates alone. The missing prerequisite is a
coordinate-safe endpoint bridge into the prepared/BIR effect stream and a real
bounded effect scan over that interval.

## Parked State

The endpoint bridge portion is available for supported rows through
`bir::LocalArrayEndpointBridgeRecord`, and selected proof-source coordinates
are present through the selected proof-edge path surface. Production still
lacks a builder that owns the selected proof-source-to-endpoint interval and
emits comparable ordered effect-source records from prepared/BIR sources.

Resume this idea only after the lower ordered effect-source stream builder can
populate assignments/redefinitions, memory accesses, phi/alias transfers,
calls/helpers, inline asm, publications, move bundles, parallel copies, and
unknown modeled effects in one comparable stream, failing closed when any
effect source cannot be ordered or modeled.

## In Scope

- Identify the authoritative prepared/BIR address-derivation endpoint for a
  local-array producer row keyed by `lir_producer_lookup_key` and
  `lir_producer_instruction_index`.
- Preserve `lir_producer_instruction_index` as a LIR producer-site coordinate;
  do not treat it as a prepared traversal index or BIR instruction index.
- Define the proof-source-to-endpoint interval boundaries used by the lower
  effect scan.
- Scan the bounded interval for assignments/redefinitions, phi/alias transfers,
  calls/helpers, inline asm, publications, move bundles, parallel copies, and
  unknown modeled effects.
- Publish a truthful endpoint/effect result that idea 494 or its successor can
  consume to distinguish available same-value/no-clobber facts from precise
  unavailable statuses.
- Add focused coverage for a bridged no-clobber representative and fail-closed
  representatives for missing endpoint, clobber, alias, unknown effect, and
  coordinate confusion.

## Out Of Scope

- Populating idea 489 proof facts directly.
- Populating idea 486 checker inputs directly.
- Idea 490 path/no-clobber certification beyond proving this lower endpoint
  and effect-scan prerequisite.
- Idea 484 packaging, scalar local-load consumption, or RV64/MIR lowering.
- Rewriting selected proof-edge path collection from idea 493.
- Treating selected-path availability, final homes, target behavior, testcase
  names, or synthetic bridge flags as proof of same-value/no-clobber.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- The endpoint bridge maps each supported dynamic local-array producer row to
  the matching ordered prepared/BIR address-derivation endpoint, or records a
  precise unavailable status.
- The implementation never uses `lir_producer_instruction_index` as a prepared
  traversal index, BIR instruction index, or ordered effect endpoint by itself.
- The bounded effect scan covers assignments/redefinitions, phi/alias
  transfers, calls/helpers, inline asm, publications, move bundles, parallel
  copies, and unknown modeled effects along the selected interval.
- Focused tests or probes prove at least one bridged no-clobber representative
  and fail-closed behavior for missing endpoint, clobber, alias, unknown
  effect, and coordinate-confusion cases.
- Residual disposition states whether idea 494 can resume publishing available
  interval facts or whether another lower owner remains first.

## Reviewer Reject Signals

Reject patches that:

- infer endpoint order or no-clobber from testcase names, block labels, value
  names, dump order, final homes, RV64 target behavior, selected path
  availability, or synthetic bridge flags;
- treat `lir_producer_instruction_index` as a prepared traversal index, BIR
  instruction index, or ordered effect endpoint;
- publish available same-value/no-clobber without a real bounded effect scan
  over the bridged prepared/BIR endpoint interval;
- skip calls/helpers, inline asm, publications, move bundles, parallel copies,
  phi/alias transfers, or unknown modeled effects when the selected interval
  can cross them;
- populate idea 489 proof facts, idea 486 checker inputs, idea 490
  certification, idea 484 packaging, scalar local-load consumption, or RV64/MIR
  lowering as part of this lower bridge slice;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress;
- retain the old selected-path-only or LIR-coordinate-only failure mode behind
  renamed helpers or status fields.
