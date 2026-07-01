# BIR Index Range Proof And Path-Dominance Carrier

Status: Open
Type: BIR semantic proof carrier/producer prerequisite idea
Parent: `ideas/closed/485_bir_local_array_address_derivation_index_range_authority_carrier.md`
Source Evidence:
- `build/agent_state/485_step3_local_array_carrier_producer/summary.md`
- `build/agent_state/485_step4_residual_disposition/disposition.md`
Owning Layer: BIR semantic index range proof and path/dominance authority
Closed By: lifecycle review after Step 4
Reopened After: ideas/closed/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md

## Completion Notes

Idea 486 is complete for the independent dynamic local-array index range proof
metadata/status surface.

Step 3 added:

- `LocalArrayRangeProofStatus`
- `LocalArrayRangeProofSourceKind`
- `LocalArrayRangeProofPredicate`
- `LocalArrayIndexRangeProofInputs`
- `LocalArrayIndexRangeProofRecord`
- `evaluate_local_array_index_range_proof`

The checker is keyed by idea 485 local-array element-path records and accepts
complete synthetic proof inputs while failing closed for missing proof,
missing bounds, unsupported predicate roles, path invalidity, non-dominating
proofs, uncovered consumer paths, missing no-clobber facts, clobbered indices,
and raw-shape-only evidence.

Completed evidence:

- `build/agent_state/486_step1_index_range_proof_inputs/audit.md`
- `build/agent_state/486_step2_range_proof_path_dominance_contract/contract.md`
- `build/agent_state/486_step3_range_proof_path_dominance_carrier/summary.md`
- `build/agent_state/486_step4_residual_disposition/disposition.md`

## Handoff

The exact follow-up is:

`ideas/open/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md`

Required follow-up scope:

- populate real proof-source records from control-flow branch/compare facts;
- prove that a proof branch/path covers a later dynamic GEP/access consumer;
- prove index no-clobber or same-value from proof source to consumer;
- update real dynamic local-array element paths from
  `missing_index_range_proof` only after every real proof input exists.

## Residual Disposition

Real dynamic row availability was previously blocked by missing real proof
facts. Idea 489 now publishes `local_array_proof_facts` from production
`local_array_index_range_proofs`, so reopened 486 should populate checker inputs
from that proof-fact surface.

The reopened work must keep the original checker/status carrier boundary:
consume proof facts, preserve fail-closed statuses, and do not add idea 485
carrier consumer changes, idea 484 packaging, scalar local-load consumption,
RV64/MIR lowering, or any raw loop-shape/value-name/testcase-name inference.

`MemoryDynamicArrayRangeVerdict::BoundedByElementCount` remains insufficient by
itself because it lacks proof source, path/dominance, and no-clobber authority.

## Reopened Lifecycle Disposition

Idea 489 Step 6 says checker input population can resume:

- `local_array_proof_facts` publishes `Available` only from matching production
  range certificates for the same dynamic local-array producer;
- fail-closed representatives remain distinguishable for missing/non-available
  proof facts, selected-path-only or interval-only inference, unsupported
  boundary, missing coordinate, clobber, alias/phi ambiguity, unknown effect,
  non-covering path, non-dominating/non-guarding proof, and coordinate
  confusion;
- idea 486 should consume `local_array_proof_facts` instead of re-deriving
  availability from `local_array_index_range_proofs`, selected paths, interval
  effects, endpoint bridges, final homes, raw testcase shape, or synthetic
  effect inputs.

## Validation

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Both passed in the original close. The lifecycle retry that reopened 486
generated `test_after.log` and ran the regression-guard self-comparison against
the backend `test_before.log`; the guard stayed at `328/328` passed.

## Reviewer Reject Signals

Reject reopening this source as progress if the change:

- claims real dynamic row availability without populated proof-source,
  path/dominance, and no-clobber facts;
- infers index range from loop shape, variable names, testcase names, dump
  order, final homes, or RV64 target behavior;
- combines this completed checker/status carrier with scalar local-load
  consumption, idea 484 packaging, RV64 lowering, or broad generic range
  analysis;
- weakens unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
