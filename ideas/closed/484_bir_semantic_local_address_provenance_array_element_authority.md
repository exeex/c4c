# BIR Semantic Local-Address Provenance And Array-Element Access Authority

Status: Closed
Type: BIR semantic producer prerequisite idea
Parent: `ideas/closed/483_bir_semantic_local_memory_scalar_load_producer.md`
Source Evidence:
- `build/agent_state/483_step1_corrected_local_load_search/audit.md`
- `build/agent_state/483_step1_corrected_local_load_search/local_load_rows.tsv`
- `review/483_step1_local_load_route_review.md`
Owning Layer: BIR semantic local-address/provenance producer
Closed By: lifecycle review after Step 4
Reopened After: ideas/closed/486_bir_index_range_proof_path_dominance_carrier.md
Closed After Reopen: lifecycle review after Step 6

## Completion Notes

Idea 484 is complete for local-address/array-element provenance authority
packaging.

The runbook audited local-address and array-element access evidence, defined a
sound local-address/array-element provenance authority contract, then proved the
current BIR/HIR local-memory lowering data cannot implement that contract
without raw-shape inference. It was later reopened after the lower authority
chain closed, and Step 5 published the production
`local_array_local_address_provenances` surface from matching checker inputs.

Completed evidence:

- `build/agent_state/484_step1_local_address_array_element_authority/audit.md`
- `build/agent_state/484_step2_local_address_provenance_contract/contract.md`
- `build/agent_state/484_step3_local_address_authority_producer/blocker.md`
- `build/agent_state/484_step4_residual_disposition/disposition.md`
- `build/agent_state/484_step5_local_address_provenance_from_checker_inputs/summary.md`
- `build/agent_state/484_step6_residual_disposition_after_local_address_provenance/disposition.md`

No scalar-load consumption, RV64/MIR lowering, expectation rewrite, or
testcase-specific shortcut is selected.

## Handoff

The exact lower-level follow-up is:

`ideas/open/485_bir_local_array_address_derivation_index_range_authority_carrier.md`

Required follow-up scope:

- durable source-object identity for local arrays and local compound literals;
- explicit array-decay or local-address derivation identity and site;
- ordered element paths;
- dynamic index identities and range proofs with proof source plus
  path/dominance validity;
- element layout/range and scalar in-bounds status;
- pointer-to-local-element provenance status;
- unavailable statuses for missing object, missing derivation, missing
  index/range, out-of-bounds, aggregate/member, integer-pointer round-trip,
  global, variadic/runtime, unsupported type, bootstrap, raw-shape-only, and
  target-only cases.

## Final Residual Disposition

No idea-484 blocker remains for local-address provenance packaging. Reopened
Step 5 publishes `local_array_local_address_provenances` only from matching
available `local_array_index_range_checker_inputs`, with matching source
object, derivation, element path, dynamic index, scalar in-bounds layout, and
producer coordinate identity.

Idea 483 scalar local-load production can resume by consuming
`local_array_local_address_provenances`. It should not re-derive provenance
from checker inputs, proof facts, range certificates, selected paths, interval
effects, endpoint bridges, final homes, raw testcase shape, synthetic effect
inputs, or target behavior.

Remaining boundaries:

- `pr38048-1.c` `a#L1[i#L3][0]` remains unavailable until the lower-level
  carrier provides source-object, derivation, index/range, and provenance facts.
- `pr22098-1.c` non-round-trip compound-literal path remains potential later
  evidence after compound-literal object identity is explicit.
- `pr22098-1.c` integer-pointer round trip remains rejected.
- `multi-ix.c` remains boundary evidence only until direct local-array site
  separation from variadic/va_arg/memset ownership is available.
- RV64/MIR lowering remains out of scope.

## Reopened Lifecycle Disposition

Idea 486 Step 6 says local-address/array-element packaging can resume:

- `local_array_index_range_checker_inputs` publishes available checker records
  only from matching available `local_array_proof_facts`;
- fail-closed representatives remain distinguishable for missing/non-available
  proof facts, selected-path-only or interval-only inference, unsupported
  boundary, missing coordinate, clobber, alias/phi ambiguity, unknown effect,
  non-covering path, non-dominating/non-guarding proof, operand-role mismatch,
  and coordinate confusion;
- idea 484 should consume `local_array_index_range_checker_inputs` instead of
  re-deriving availability from proof facts, range certificates, selected
  paths, interval effects, endpoint bridges, final homes, raw testcase shape,
  or synthetic effect inputs.

## Validation

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Both passed in the original close. The lifecycle retry that reopened 484
generated `test_after.log` and ran the regression-guard self-comparison against
the backend `test_before.log`; the guard stayed at `328/328` passed. The final
close/switch retry to idea 483 used the corrected self-comparison command and
preserved the same `328/328` result.

## Reviewer Reject Signals

Reject reopening this source as progress if the change:

- implements local-address provenance by inferring from route-local
  `element_slots`, `base_index`, lowered index values, type text, testcase
  names, value names, dump order, final homes, or target fallback behavior;
- consumes scalar local loads or changes RV64/MIR lowering before lower-level
  authority records exist;
- treats integer-pointer round trips as valid local provenance;
- folds global, aggregate/member, variadic/runtime, F128, complex/vector,
  volatile/atomic, bootstrap, scalar-load, or target-lowering work into this
  closed negative result;
- changes expectations, unsupported markers, allowlists, pass/fail accounting,
  runtime comparison, or baseline logs as proof of progress.
