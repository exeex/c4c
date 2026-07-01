# Dynamic Local-Array LIR Producer Path/No-Clobber Certificate

Status: Closed
Type: Routed certificate investigation
Parent: `ideas/closed/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md`
Closed By: lifecycle split to `ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md`
Source Evidence:
- `build/agent_state/489_step3_proof_population_route/route.md`
- `build/agent_state/489_step4_residual_disposition/disposition.md`
- `build/agent_state/490_step3_certificate_producer_route/route.md`
- `build/agent_state/490_step4_residual_disposition/disposition.md`
Owning Layer: BIR/prepared path coverage and no-clobber certificate investigation keyed to LIR producer coordinates

## Closure Summary

Idea 490 is complete as a routed certificate investigation, not as an
implemented certificate producer.

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

## Follow-Up

The next active owner is:

`ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md`

That follow-up owns the first missing certificate family:

- proof branch/compare identity;
- selected proof edge/outcome;
- proof-source-to-LIR-producer path coverage;
- dominance or guard validity under the selected outcome;
- explicit statuses for missing selected edge, missing path, non-covering path,
  non-dominating/non-guarding proof, unsupported boundary, missing same-block
  ordering, and prepared/BIR coordinate confusion.

After that exists, a later separate owner should classify dynamic-index
interval effects and no-clobber facts over the certified path.

## Preserved Boundaries

- Do not populate idea 486 checker inputs directly from this routed result.
- Do not resume idea 489 proof population until selected proof-edge path
  certification and dynamic-index interval effect classification both exist.
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
