# BIR Dynamic Local-Array Proof Population From LIR Coordinates

Status: Open
Type: BIR semantic proof population producer idea
Parent: `ideas/closed/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md`
Source Evidence:
- `build/agent_state/488_step3_consumer_coordinate_exposure/summary.md`
- `build/agent_state/488_step4_residual_disposition/disposition.md`
Owning Layer: BIR semantic proof-source/path/no-clobber population using LIR producer coordinates
Previously Closed By: lifecycle review after Step 4
Reopened After: ideas/closed/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md

## Completion Notes

Idea 489 was closed as a routed proof-population investigation, not as an
implemented real proof-population producer.

The runbook established that the committed idea 488 `lir_producer_*` coordinate
surface is sufficient as a binding key, and that prepared fused branch/compare
facts provide candidate proof sources. It also established that real dynamic
local-array proof population cannot proceed without a lower certificate
producer for proof-source path coverage and same-value/no-clobber interval facts
keyed to the same `lir_producer_*` site.

Completed evidence:

- `build/agent_state/489_step3_proof_population_route/route.md`
- `build/agent_state/489_step4_residual_disposition/disposition.md`

## Handoff

The exact follow-up was:

`ideas/closed/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md`

Required follow-up scope:

- publish a certificate keyed by `lir_producer_*` fields or the `lir-producer:`
  lookup key;
- bind prepared branch/compare proof source, selected edge/outcome, path
  coverage, and dominance/guard validity to that LIR producer site;
- prove the dynamic index remains the same over the interval;
- model clobber effects for redefinitions, phi/alias, calls/helpers,
  inline asm, publications, move bundles, and parallel copies;
- provide fail-closed statuses that can feed idea 486 checker inputs without
  broadening the checker vocabulary.

## Residual Disposition

The lower certificate now exists. Reopened 489 should resume proof population
by consuming `local_array_index_range_proofs` as the certificate surface for
dynamic local-array proof facts.

Remaining downstream boundaries:

- idea 484 packaging remains blocked for dynamic rows;
- scalar local-load production remains blocked;
- RV64/MIR lowering remains out of scope;
- idea 488 coordinate semantics remain unchanged:
  `lir_producer_instruction_index` is an LIR producer-site coordinate, not a
  prepared traversal/BIR instruction coordinate.

## Reopened Lifecycle Disposition

Idea 490 Step 6 says proof population can resume:

- `local_array_index_range_proofs` publishes `Available` only from matching
  lower selected-path and interval-effect authorities for the same dynamic
  local-array producer;
- fail-closed representatives are covered for missing selected path,
  selected-path-only evidence, missing interval effect, interval-only evidence,
  clobber, alias/phi ambiguity, unknown effect, missing coordinate,
  unsupported boundary, non-covering path, non-dominating/non-guarding proof,
  and coordinate confusion;
- idea 489 should consume those certificate records instead of re-deriving
  selected-path coverage, no-clobber, endpoint bridges, final homes, raw
  testcase shape, or synthetic effect inputs.

Reopened scope is limited to populating real dynamic local-array proof facts
from `local_array_index_range_proofs`, then recording whether idea 486 checker
input population can resume.

## Validation

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Both passed in the original routed close. The later close/switch retry that
reopened 489 regenerated `test_after.log` and ran the regression-guard
self-comparison against the backend `test_before.log`; the guard stayed at
`328/328` passed.

## Reviewer Reject Signals

Reject reopening this source as progress if the change:

- implements proof population by inferring from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or RV64 target behavior;
- marks dynamic rows available without a certificate for path coverage,
  dominance/guard validity, and dynamic-index same-value/no-clobber;
- treats `lir_producer_instruction_index` as a prepared traversal/BIR
  instruction coordinate without a separate conversion owner;
- combines proof population with idea 484 packaging, scalar local-load
  consumption, RV64/MIR lowering, prepared traversal conversion, or broad
  generic range analysis;
- changes expectations, unsupported markers, allowlists, pass/fail accounting,
  runtime comparison, or baseline logs as proof of progress.
