# BIR Call Argument ABI Scalar Metadata Coherence

Status: Open
Type: Producer contract repair idea
Parent: `ideas/open/431_prepared_aggregate_abi_contract_review.md`
Source Evidence:
- `build/agent_state/431_step1_aggregate_abi_audit/evidence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/classification.tsv`
- `build/agent_state/431_step2_abi_fact_coherence/classification.md`
Owning Layer: Raw BIR call ABI producer and producer-owned verifier contracts

## Goal

Ensure raw and prepared BIR call-argument ABI metadata does not publish
aggregate-only `sret` or `byval` facts on scalar call arguments.

## Why This Exists

The prepared aggregate ABI review classified `src/20010224-1.c` and
`src/20020506-1.c` as scalar ABI metadata producer defects, not aggregate
lowering inputs. Raw and prepared BIR currently render impossible aggregate ABI
suffixes on scalar `i16` arguments:

- `ba_compute_psd(i16 byval(size=14, align=8462384025005154658) 0)`
- `test3(i16 sret(size=5, align=220997051764) %t20, i32 1)`
- `test4(i16 sret(size=5, align=225292019060) %t28, i32 1)`

Later prepared call planning repairs the actual movements as scalar GPR facts,
but the raw `bir::CallInst::arg_abi` contract is still incoherent and can
mislead target-lowering work.

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
