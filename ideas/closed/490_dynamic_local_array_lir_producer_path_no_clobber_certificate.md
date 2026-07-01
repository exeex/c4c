# Dynamic Local-Array LIR Producer Path/No-Clobber Certificate

Status: Closed
Type: Routed certificate investigation
Parent: `ideas/closed/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md`
Previously Closed By: lifecycle split to `ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md`
Source Evidence:
- `build/agent_state/489_step3_proof_population_route/route.md`
- `build/agent_state/489_step4_residual_disposition/disposition.md`
- `build/agent_state/490_step3_certificate_producer_route/route.md`
- `build/agent_state/490_step4_residual_disposition/disposition.md`
Owning Layer: BIR/prepared path coverage and no-clobber certificate investigation keyed to LIR producer coordinates

## Closure Summary

Idea 490 is complete.

The reopened implementation scope is satisfied: production-backed dynamic
local-array range proof certificates are now published through
`local_array_index_range_proofs` by consuming matching lower selected-path and
interval-effect authorities.

Earlier in the route, idea 490 was complete only as a routed certificate
investigation, not as an implemented certificate producer.

The runbook established the required LIR producer path/no-clobber certificate
shape and classified why current prepared/BIR surfaces cannot implement it
soundly. The first missing producer is a selected proof-edge path certificate:
a durable record that selects a proof edge/outcome and certifies path coverage,
dominance or guard validity, same-block ordering safety, and proof-source
binding for a specific `lir_producer_lookup_key`.

## Completed Work

- Audited current `lir_producer_*` path records, prepared branch/compare facts,
  dominance/reachability helpers, dynamic-index identity, and effect inputs.
- Defined the larger path/no-clobber certificate contract and fail-closed
  statuses.
- Routed implementation because selected proof-edge/path/order certification is
  missing.
- Preserved the later dynamic-index interval effect/no-clobber classifier as a
  separate owner after selected proof-edge path certification exists.

## Historical Follow-Up

The routed follow-up was:

`ideas/closed/491_dynamic_local_array_selected_proof_edge_path_certificate.md`

That follow-up owns the first missing certificate family:

- proof branch/compare identity;
- selected proof edge/outcome;
- proof-source-to-LIR-producer path coverage;
- dominance or guard validity under the selected outcome;
- explicit statuses for missing selected edge, missing path, non-covering path,
  non-dominating/non-guarding proof, unsupported boundary, missing same-block
  ordering, and prepared/BIR coordinate confusion.

After that existed, later separate owners classified dynamic-index interval
effects and no-clobber facts over the certified path. Those lower owners are
now complete.

## Final Reopened Completion

Reopened 490 consumed the completed lower owner chain and published the
certificate surface required by idea 489:

- `bir::Function::local_array_index_range_proofs`;
- a BIR certificate evaluator requiring matching
  `local_array_selected_proof_edge_paths` and `local_array_interval_effects`;
- `prepare::populate_local_array_index_range_proofs`, wired into
  `BirPreAlloc::publish_contract_plans` after interval-effect publication;
- fail-closed coverage for missing selected path, selected-path-only evidence,
  missing interval effect, interval-only evidence, clobber, alias/phi,
  unknown effect, missing coordinate, unsupported boundary, non-covering path,
  non-dominating/non-guarding proof, and coordinate confusion.

Final evidence:

- `build/agent_state/490_step5_range_proof_from_lower_authorities/summary.md`
- `build/agent_state/490_step6_residual_disposition_after_lower_authorities/disposition.md`

## Reopened Lifecycle Disposition

Idea 490 is reopened after the lower owner chain completed:

- idea 491 selected proof-edge path certificate route;
- idea 492 selected proof-edge path record/status API;
- idea 493 selected proof-edge path collector/population;
- idea 494 interval effect classifier;
- idea 497 endpoint bridge effect scan;
- idea 498 ordered effect-source stream builder.

The resumed implementation scope is no longer to rediscover the missing lower
owners. It is to publish the path/no-clobber certificate facts that idea 489
routed here, using the now-populated lower authorities:

- `local_array_selected_proof_edge_paths` for selected proof edge, path
  coverage, dominance or guard validity, and LIR producer binding;
- `local_array_interval_effects` for dynamic-index same-value/no-clobber over
  the selected proof-source-to-endpoint interval.

The first resumed packet should bridge those authorities into the existing
dynamic local-array range proof status surface without inferring availability
from selected paths alone, endpoint bridges alone, final homes, target
behavior, raw testcase shape, or synthetic effect inputs.

This resumed implementation is now complete. Idea 489 can resume proof
population by consuming `local_array_index_range_proofs`.

## Preserved Boundaries

- Do not populate idea 486 checker inputs directly from this routed result.
- Do not resume idea 489 proof population from this source until this reopened
  certificate producer publishes explicit range proof facts.
- Do not treat `lir_producer_instruction_index` as a prepared traversal or BIR
  instruction coordinate.
- Do not route this as idea 484 packaging, scalar local-load consumption,
  RV64/MIR lowering, generic range analysis, expectation rewrites, allowlists,
  or baseline/log changes.

## Validation

Close/split lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Result: pending in successor lifecycle patch.
Final successor patch result: passed. Regression sanity used the unchanged
canonical log as a lifecycle-only close gate (`328/328` before and after).
Reopened close/switch gate result: passed with backend self-comparison at
`328/328`.

## Reviewer Reject Signals

Reject any attempt to reopen this route as progress if it:

- infers path coverage or no-clobber from branch proximity, loop shape, value
  names, testcase names, dump order, final homes, or RV64 target behavior;
- marks proof facts available without a durable selected proof-edge certificate
  keyed to `lir_producer_*`;
- treats LIR producer instruction index as a prepared traversal/BIR instruction
  index without a separate conversion owner;
- folds interval effect/no-clobber population into the selected edge path
  certificate without explicit source-idea selection;
- weakens unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
