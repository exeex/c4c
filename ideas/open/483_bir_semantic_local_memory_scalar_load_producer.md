# BIR Semantic Local-Memory Scalar Load Producer

Status: Open
Type: Focused BIR semantic producer implementation idea
Parent: `ideas/closed/422_bir_semantic_producer_high_impact_cleanup.md`
Source Evidence:
- `build/agent_state/422_step2_bir_producer_buckets/bucket_table.tsv`
- `build/agent_state/422_step3_first_producer_packet/decision.md`
- `build/agent_state/422_step4_residual_disposition/disposition.md`
Owning Layer: BIR semantic local-memory load producer
Closed By: lifecycle review after corrected Step 1 search
Reopened After: ideas/closed/484_bir_semantic_local_address_provenance_array_element_authority.md

## Completion Notes

Idea 483 is closed as a routed blocker, not as an implemented scalar local-load
producer.

The source goal was to find and implement an ordinary scalar local-memory load
producer packet that did not depend on GEP, pointer/provenance, aggregate/member,
byval/va_arg, volatile/atomic, complex, vector, F128, bootstrap, or target
fallback inference. Corrected Step 1 searched the full 79-row local-memory load
bucket and found no such representative.

Primary evidence:

- `build/agent_state/483_step1_corrected_local_load_search/audit.md`
- `build/agent_state/483_step1_corrected_local_load_search/local_load_rows.tsv`
- `build/agent_state/483_step1_corrected_local_load_search/hir_direct_local_array_candidates.tsv`
- `build/agent_state/483_step1_corrected_local_load_search/hir_plain_local_rhs_candidates.tsv`
- `review/483_step1_local_load_route_review.md`

## Route Decision

Do not advance idea 483 to Step 2. The first prerequisite owner is:

`ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md`

That producer must expose local object, array decay, index/offset, element
layout, and pointer-to-local-element provenance facts before a scalar
local-load producer can safely consume rows such as `pr38048-1.c` or the
non-round-trip part of `pr22098-1.c`.

## Residual Disposition

The following representative classes previously remained fail-closed for 483:

- `src/pr22098-{1,2,3}.c`: compound literal array element through pointer or
  integer-pointer round-trip.
- `src/pr38048-1.c`: local scalar element load through local pointer
  `a = mat`, requiring array-decay/address/provenance/GEP facts.
- `src/multi-ix.c`: direct scalar array evidence exists, but the row's latest
  failure is variadic/va_arg contaminated.
- `src/pr33870-1.c` and `src/pr33870.c`: pointer values loaded from local
  pointer arrays.
- `src/pr15262.c` and `src/pr15296.c`: aggregate/member or pointer-derived
  struct/union access.
- Union, complex, F128, object-representation, global, runtime/call, variadic,
  and bootstrap-adjacent rows.

Idea 484 now publishes `local_array_local_address_provenances` for the local
array element provenance prerequisite. Reopened 483 should consume that surface
for scalar local-load production and should not re-derive provenance from
checker inputs, proof facts, range certificates, selected paths, interval
effects, endpoint bridges, final homes, raw testcase shape, synthetic effect
inputs, or target behavior.

## Reopened Lifecycle Disposition

Idea 484 Step 6 says scalar local-load production can resume:

- `local_array_local_address_provenances` publishes available records only from
  matching available checker inputs with matching source object, derivation,
  element path, dynamic index, scalar in-bounds layout, and producer coordinate
  identity;
- fail-closed representatives remain distinguishable for missing/non-available
  checker input, missing source object, missing derivation, missing dynamic
  index, missing range, out-of-bounds, aggregate/member, integer-pointer
  round-trip, global source object, variadic/runtime boundary, unsupported
  type, bootstrap boundary, raw-shape-only evidence, target-only evidence, and
  coordinate confusion;
- idea 483 should consume `local_array_local_address_provenances` instead of
  re-deriving local-address provenance.

## Closed Scope

Closed as complete for:

- corrected full-bucket representative search;
- rejection of pointer/provenance-dependent route drift;
- identification of the prerequisite local-address/provenance and array-element
  access authority owner.

Not closed as:

- implementation of scalar local-memory load producer facts;
- pointer/provenance or array-element access authority completion;
- approval to infer local-load facts in RV64/MIR.

## Validation

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Both passed in the original routed close. The lifecycle retry that reopened 483
generated `test_after.log` with the regression-guard self-comparison against
the backend `test_before.log`; the guard stayed at `328/328` passed.

## Reviewer Reject Signals

Reject reopening this source as progress if the change:

- proceeds with scalar local-load implementation without first providing the
  local-address/provenance and array-element authority prerequisite;
- uses `pr22098-1.c`, `pr38048-1.c`, or similar pointer/array-decay rows as
  ordinary scalar local-load contract drivers under idea 483;
- infers local object, element offset, range, layout, or provenance from
  testcase names, value names, raw dump order, final homes, or RV64 target
  fallback behavior;
- broadens this closed blocker into GEP, store, call, memcpy/memset, alloca,
  bootstrap, aggregate/member, pointer/provenance, byval/va_arg, volatile,
  atomic, complex, vector, or F128 work without a separate source idea;
- changes expectations, unsupported markers, allowlists, pass/fail accounting,
  runtime comparison, or baseline logs as proof of progress.
