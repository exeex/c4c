Status: Active
Source Idea Path: ideas/open/431_prepared_aggregate_abi_contract_review.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close-Readiness Review

# Current Packet

## Just Finished

Completed Step 3, Produce Follow-Up Disposition.

Durable follow-up ownership is now recorded under separate open source ideas:

- `ideas/open/434_bir_call_arg_abi_scalar_metadata_coherence.md`: producer
  contract repair for scalar call arguments that publish impossible aggregate
  `sret`/`byval` ABI metadata in raw `bir::CallInst::arg_abi`.
- `ideas/open/435_rv64_coherent_aggregate_sret_call_storage_lowering.md`:
  later RV64 target lowering for coherent aggregate `sret` rows only after
  producer contracts are clean, initially `src/20000917-1.c` and
  `src/20020206-1.c`.
- `ideas/open/436_pr88904_sret_object_home_producer_review.md`: producer
  review for `src/pr88904.c` `sret_param` size/alignment/object-home
  coherence before that row can join aggregate lowering.

Step 3 disposition:

- Raw scalar ABI metadata coherence is split to producer idea 434.
- Coherent RV64 aggregate lowering candidates are split to target idea 435 and
  explicitly exclude scalar metadata defects and `pr88904` until producer
  review completes.
- `pr88904` size/alignment/object-home producer review is split to producer
  idea 436.
- The active 431 review plan retains only Step 4 close-readiness review.

## Suggested Next

Execute Step 4: Close-Readiness Review.

Review whether `ideas/open/431_prepared_aggregate_abi_contract_review.md` is
close-ready now that the aggregate bucket has been classified and all durable
follow-up ownership has been split into ideas 434, 435, and 436. Close only if
the source idea's acceptance criteria are satisfied and the required close gate
passes.

## Watchouts

- Do not implement RV64 aggregate lowering in this review plan.
- Do not treat aggregate rows as scalar call publication.
- Do not infer missing aggregate size/alignment/storage/return slot/byval copy
  facts in RV64.
- Keep coherent aggregate `sret` rows (`20000917-1`, `20020206-1`) separate
  from suspicious scalar ABI metadata rows (`20010224-1`, `20020506-1`).
- `pr88904` has both real `sret` facts and separate inline-asm/store
  publication residuals; do not collapse those into aggregate ABI lowering.
- Do not add a verifier test that only validates the already-repaired prepared
  call move; the missing contract is raw call ABI metadata coherence.
- Keep F128, long-double, runtime-helper policy, expectation changes, and
  pass/fail accounting out of this plan.

## Proof

Step 3 lifecycle-only proof:

```sh
git diff --check
```
