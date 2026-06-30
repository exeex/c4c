# BIR Call Argument ABI Scalar Metadata Coherence

Status: Closed
Type: Producer contract repair idea
Parent: `ideas/closed/431_prepared_aggregate_abi_contract_review.md`
Source Evidence:
- `build/agent_state/431_step1_aggregate_abi_audit/evidence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/classification.tsv`
- `build/agent_state/431_step2_abi_fact_coherence/classification.md`
Owning Layer: Raw BIR call ABI producer and producer-owned verifier contracts
Closed: Step 4 close-readiness review

## Goal

Ensure raw and prepared BIR call-argument ABI metadata does not publish
aggregate-only `sret` or `byval` facts on scalar call arguments.

## Why This Exists

The prepared aggregate ABI review classified `src/20010224-1.c` and
`src/20020506-1.c` as scalar ABI metadata producer defects, not aggregate
lowering inputs. Raw and prepared BIR rendered impossible aggregate ABI suffixes
on scalar `i16` arguments:

- `ba_compute_psd(i16 byval(size=14, align=8462384025005154658) 0)`
- `test3(i16 sret(size=5, align=220997051764) %t20, i32 1)`
- `test4(i16 sret(size=5, align=225292019060) %t28, i32 1)`

Later prepared call planning repaired the actual movements as scalar GPR facts,
but the raw `bir::CallInst::arg_abi` contract was incoherent and could mislead
target-lowering work.

## In Scope

- Audit the producer path that fills `bir::CallInst::arg_abi` for scalar call
  arguments.
- Repair scalar call arguments so they do not carry aggregate-only
  `sret_pointer` or `byval_copy` metadata.
- Add or extend a producer-owned verifier/classifier contract that can fail
  closed on raw call ABI metadata incoherence before target lowering consumes
  it.
- Use `src/20010224-1.c` and `src/20020506-1.c` as representatives, while
  implementing a semantic scalar-vs-aggregate ABI rule.

## Out Of Scope

- Implementing RV64 aggregate `sret` or `byval` lowering.
- Treating the existing prepared call-plan repair as sufficient proof that raw
  BIR ABI metadata is coherent.
- Named-testcase rewrites that only hide the impossible suffixes for the two
  representatives.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.
- F128, long-double, runtime-helper, inline-asm, select, or store-publication
  residuals.

## Acceptance Criteria

- Scalar call arguments no longer publish aggregate-only `sret`/`byval` ABI
  metadata in raw or prepared BIR, or a producer-owned verifier fails closed
  before target lowering treats those facts as coherent.
- `src/20010224-1.c` and `src/20020506-1.c` no longer appear as aggregate
  ABI lowering candidates due to bogus raw `arg_abi` metadata.
- Focused producer/verifier tests cover scalar arguments carrying impossible
  aggregate ABI flags.
- Any object-route progress is attributed only to the producer repair, not to
  aggregate target lowering.

## Completion Notes

Step 2 added raw call-argument ABI coherence verifier coverage. Step 3 repaired
production so scalar non-pointer arguments no longer publish aggregate-only
`sret_pointer` or `byval_copy` metadata while pointer aggregate `sret`/`byval`
metadata remains preserved by focused tests.

Final evidence:

- `build/agent_state/434_step3_scalar_arg_abi_repair/summary.txt`
- `build/agent_state/434_step3_scalar_arg_abi_repair/*.after.dump_bir.out`
- `build/agent_state/434_step3_scalar_arg_abi_repair/*.after.dump_prepared_bir.out`

Representative after dumps show:

- `src/20010224-1.c` raw/prepared BIR prints
  `bir.call void ba_compute_psd(i16 0)`.
- `src/20020506-1.c` raw/prepared BIR prints the `test3`/`test4` scalar
  arguments as plain `i16`.
- `rg -n "i16 (byval|sret)\(" build/agent_state/434_step3_scalar_arg_abi_repair/*.after.dump_*`
  has no matches.

Validation:

- Backend proof was run and rolled to `test_before.log`.
- Close-time regression guard passed on `test_before.log` versus
  `test_after.log` with non-decreasing pass counts:
  `327 passed, 0 failed` before and after.
- `git diff --check` passed at close.

No RV64 aggregate lowering was performed by this idea.

## Reviewer Reject Signals

- Reject testcase-shaped filters or helper shortcuts that only special-case
  `ba_compute_psd`, `test3`, `test4`, `20010224-1`, or `20020506-1`.
- Reject a route that leaves raw `bir::CallInst::arg_abi` incoherent while
  claiming the later prepared call-plan repair solved the producer contract.
- Reject expectation downgrades, unsupported-marker changes, or pass/fail
  accounting edits as progress.
- Reject broad ABI rewrites that also pull in RV64 aggregate lowering, F128,
  inline-asm, select, or unrelated calling-convention work.
- Reject classification-only edits claimed as capability progress unless this
  idea is explicitly retired as a review-only disposition.
