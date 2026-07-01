# Dynamic Local-Array LIR Producer Interval Effect Classifier

Status: Closed
Type: BIR/prepared dynamic-index interval effect classifier idea
Parent: `ideas/closed/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md`
Source Evidence:
- `build/agent_state/493_step3_collector_population/summary.md`
- `build/agent_state/493_step4_residual_disposition/disposition.md`
Owning Layer: Dynamic-index same-value/no-clobber interval classifier keyed to selected proof-edge path records

## Goal

Classify whether the dynamic index value used by a local-array element path and
its selected proof compare remains same-value/no-clobber over the certified
proof-source-to-LIR-producer interval.

## Why This Exists

Idea 493 populated selected proof-edge path records, so the branch/edge/path
and dominance/guard side of the proof is now explicit. Full dynamic local-array
proof population still needs interval facts showing that the dynamic index is
not redefined, aliased, clobbered, or otherwise invalidated between the proof
source and LIR producer site.

## In Scope

- Audit dynamic-index identity sources in local-array element path records and
  prepared proof branch/compare records.
- Bind interval classification to populated selected proof-edge path records by
  exact `lir_producer_lookup_key`, proof branch identity, selected outcome, and
  dynamic-index operand role.
- Define interval start/end semantics from proof source to LIR producer site
  without treating `lir_producer_instruction_index` as a prepared/BIR
  instruction index.
- Classify same-value/no-clobber effects for redefinitions/assignments,
  phi/alias edges, calls/helpers, inline asm, publications, move bundles, and
  parallel copies.
- Publish explicit available/unavailable interval effect facts or route to the
  exact lower owner if current prepared data cannot classify them.
- Add focused coverage for available and fail-closed interval classifications
  if implementation is selected.

## Out Of Scope

- Re-implementing selected proof-edge path record API or collector population.
- Populating idea 489 proof facts directly.
- Populating idea 486 checker inputs directly.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- LIR-to-prepared/BIR same-block instruction ordering conversion beyond
  explicit unavailable status reporting.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Step 1 identifies current sources for dynamic-index identity, proof-source
  binding, interval boundaries, and effect/no-clobber inputs, or records the
  exact lower blocker.
- The contract names the interval key, start/end semantics, same-value
  criterion, effect classes, and fail-closed statuses.
- If implementation is selected, focused tests/probes show available interval
  facts for a bounded representative and fail-closed statuses for clobber,
  alias, unknown effect, missing path/order, raw-shape-only inference,
  target/final-home-only inference, and coordinate confusion.
- Residual disposition states whether idea 490 path/no-clobber certification
  can resume or whether another lower interval/effect owner remains first.

## Resumed Lifecycle Disposition

Plan 494 previously exhausted on Step 4 and parked blocked after recording the
fail-closed interval status surface in
`build/agent_state/494_step4_residual_disposition/disposition.md`.

Closed idea 497 now supplies the lower endpoint/effect prerequisite. Its Step 5
disposition in
`build/agent_state/497_step5_residual_disposition_after_498/disposition.md`
says the classifier can resume available interval fact publication by consuming
the exact matching production `local_array_ordered_effect_source_streams`
record through the endpoint bridge.

Resumed 494 work must keep the original fail-closed statuses, but may now
publish `available` only when the stored stream consumer proves clean bounded
same-value/no-clobber evidence for the selected proof-source-to-endpoint
interval.

## Completion Notes

Idea 494 is complete after resumed Steps 5 and 6.

Completed evidence:

- `build/agent_state/494_step5_available_interval_from_stored_stream/summary.md`
- `build/agent_state/494_step6_residual_disposition_after_stored_stream/disposition.md`

The production BIR surface now publishes `local_array_interval_effects` from
selected proof-edge path records, endpoint bridge authority, and exactly one
matching production ordered effect-source stream. The available representative
and fail-closed representatives for missing/path-only streams, duplicate
streams, missing endpoint bridge, missing coordinates, unordered boundaries,
coordinate confusion, clobber, alias/phi, and unknown effects are covered.

The next lifecycle owner is reopened idea 490:

`ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md`

That owner should consume `local_array_selected_proof_edge_paths` and
`local_array_interval_effects` as lower authorities for path/no-clobber range
proof certification.

## Reviewer Reject Signals

Reject patches that:

- infer same-value/no-clobber from testcase names, block labels, value names,
  dump order, final homes, RV64 target behavior, or selected path availability
  alone;
- treat `lir_producer_instruction_index` as a prepared traversal or BIR
  instruction index;
- skip explicit classification for calls/helpers, inline asm, publications,
  move bundles, or parallel copies when the path can cross them;
- populate idea 489 proof facts or idea 486 checker inputs directly instead of
  publishing interval classifier facts;
- combine interval classification with idea 484 packaging, scalar local-load
  consumption, or RV64/MIR lowering;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
