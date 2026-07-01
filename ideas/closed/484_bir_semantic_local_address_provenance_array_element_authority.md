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

## Completion Notes

Idea 484 is closed as a negative/blocked producer result.

The runbook audited local-address and array-element access evidence, defined a
sound local-address/array-element provenance authority contract, then proved the
current BIR/HIR local-memory lowering data cannot implement that contract
without raw-shape inference.

Completed evidence:

- `build/agent_state/484_step1_local_address_array_element_authority/audit.md`
- `build/agent_state/484_step2_local_address_provenance_contract/contract.md`
- `build/agent_state/484_step3_local_address_authority_producer/blocker.md`
- `build/agent_state/484_step4_residual_disposition/disposition.md`

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

## Residual Disposition

After 485 exists, this source family may need a small packaging packet that
exposes the lower-level carrier in the Step 2 local-address/array-element
record shape. Only after that record exists should idea 483-style scalar
local-load production resume.

Remaining boundaries:

- `pr38048-1.c` `a#L1[i#L3][0]` remains unavailable until the lower-level
  carrier provides source-object, derivation, index/range, and provenance facts.
- `pr22098-1.c` non-round-trip compound-literal path remains potential later
  evidence after compound-literal object identity is explicit.
- `pr22098-1.c` integer-pointer round trip remains rejected.
- `multi-ix.c` remains boundary evidence only until direct local-array site
  separation from variadic/va_arg/memset ownership is available.
- RV64/MIR lowering remains out of scope.

## Validation

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Both passed. `test_after.log` was absent at close time, and this lifecycle task
forbade touching canonical logs, so no new after log was generated.

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
