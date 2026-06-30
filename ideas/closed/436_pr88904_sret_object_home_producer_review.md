# PR88904 SRet Object Home Producer Review

Status: Closed
Type: Producer contract review idea
Parent: `ideas/closed/431_prepared_aggregate_abi_contract_review.md`
Source Evidence:
- `build/agent_state/431_step1_aggregate_abi_audit/evidence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/classification.tsv`
- `build/agent_state/431_step2_abi_fact_coherence/classification.md`
Owning Layer: Prepared aggregate ABI producer and object-home contracts
Closed: Step 4 close-readiness review

## Goal

Reconcile `src/pr88904.c` aggregate `sret` size, alignment, and object-home
facts before the row can seed any RV64 aggregate lowering work.

## Why This Exists

`src/pr88904.c` has real aggregate `sret(size=8, align=4)` call facts, but the
prepared object metadata reports the `sret_param` object as `size=8 align=8`
while the call memory record reports `memory_size=8 memory_align=4`. The row
also contains unrelated inline-asm and store-publication residuals, so it must
not be folded into coherent aggregate lowering without producer review.

## In Scope

- Audit the prepared producer path for `sret_param` object size/alignment and
  call memory size/alignment records.
- Decide whether `align=8` on the object record and `memory_align=4` on the
  call record are coherent under the current prepared contract or a producer
  mismatch.
- Verify object-home ownership for the `sret` frame-slot path.
- Add focused contract coverage if the mismatch is a producer defect or an
  ambiguous accepted contract.
- Produce a clear disposition for whether `pr88904` can later join coherent
  RV64 aggregate `sret` lowering.

## Out Of Scope

- Implementing RV64 aggregate lowering for `pr88904`.
- Fixing the row's unrelated inline-asm, clobber, store-publication, select, or
  local-publication residuals.
- Folding `pr88904` into the coherent `20000917-1`/`20020206-1` lowering scope
  before producer facts are reconciled.
- Inferring object layout, alignment, or home ownership in RV64 from source or
  testcase identity.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Acceptance Criteria

- The `pr88904` `sret_param` object metadata and call memory records have a
  documented coherent interpretation or a producer-owned defect route.
- Focused producer contract tests cover any confirmed mismatch or ambiguous
  accepted contract.
- Follow-up disposition states whether `pr88904` is eligible for later RV64
  aggregate `sret` lowering, and which unrelated residuals remain separate.
- No target-lowering work consumes `pr88904` as coherent evidence before this
  review completes.

## Completion Notes

The producer-owned ambiguity is resolved as coherent:

- The callee hidden `sret` parameter object home is pointer storage:
  `type=ptr size=8 align=8`.
- The caller memory-return payload retains aggregate ABI facts:
  `size=8 align=4`.
- These facts describe different entities: the pointer-valued hidden parameter
  home versus the pointed-to aggregate return payload.
- Step 2 added semantic RV64 same-module aggregate `sret` coverage that guards
  this interpretation. No producer implementation repair was needed.

`pr88904` is eligible to be referenced later as coherent aggregate `sret`
producer evidence with caveats. Later RV64 lowering may use it only for the
resolved producer contract and must still consume prepared facts semantically,
not by matching `pr88904`, `foo`, object numbers, line numbers, or dump
positions.

Separately routed residuals remain outside this producer-review idea:

- inline-asm carriers with unsupported `=*imr`/`*imr` constraints
- clobber and inline-asm materialization residuals
- `store_source ... status=missing_destination_access` publication residuals
- stack-slot block-entry publication residuals
- select/local-publication residuals
- generic RV64 object-route `unsupported_instruction_fragment`
- RV64 target aggregate lowering itself
- expectation, allowlist, unsupported-marker, runtime-comparison, and
  pass/fail accounting changes

Close evidence:

- `build/agent_state/436_step1_pr88904_sret_fact_audit/classification.md`
- `build/agent_state/436_step2_pr88904_contract_coverage/summary.txt`
- `build/agent_state/436_step3_pr88904_eligibility_disposition/disposition.md`
- Backend regression guard passed with existing canonical logs:
  `test_before.log` and `test_after.log` both reported `327 passed, 0 failed`.
- `git diff --check` passed at close.

No new durable follow-up idea was created during close review because the
remaining residuals are outside this source idea and were already classified as
separate from the `sret` object-home producer contract.

## Reviewer Reject Signals

- Reject RV64 lowering that consumes the `pr88904` `sret` facts before the
  object size/alignment/home contract is reconciled.
- Reject treating inline-asm, store-publication, select, or local-publication
  residuals as part of this producer review.
- Reject named-case shortcuts that only recognize `pr88904` or `foo`.
- Reject expectation downgrades, unsupported-marker changes, or pass/fail
  accounting edits as producer contract progress.
- Reject classification-only edits claimed as capability progress unless this
  idea is explicitly closed as review-only.
