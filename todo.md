Status: Active
Source Idea Path: ideas/open/436_pr88904_sret_object_home_producer_review.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close-Readiness Review

# Current Packet

## Just Finished

Completed Step 3: produced the `pr88904` follow-up eligibility disposition
after coherent aggregate `sret` object-home coverage.

Disposition:

- `pr88904` is eligible to be referenced later as coherent aggregate `sret`
  producer evidence, with caveats.
- The producer-fact ambiguity is resolved: hidden `sret` parameter object
  homes are pointer storage (`type=ptr size=8 align=8`), while caller
  memory-return payload facts retain aggregate ABI `size=8 align=4`.
- Step 2 focused RV64 same-module coverage guards that coherent interpretation,
  so no producer repair remains in this plan.
- This disposition does not admit `pr88904` as a single immediate RV64
  aggregate lowering acceptance row; later target lowering must still consume
  prepared facts semantically.
- Separately routed residuals remain outside this idea: inline-asm `=*imr`/
  `*imr` carriers, clobber/inline-asm materialization residuals,
  `missing_destination_access` store-publication facts, stack-slot block-entry
  publication, select/local-publication residuals, generic
  `unsupported_instruction_fragment`, and target aggregate lowering.

Artifacts:

- `build/agent_state/436_step3_pr88904_eligibility_disposition/disposition.md`

## Suggested Next

Execute Step 4: Close-Readiness Review.

First bounded packet:

- Validate that Step 1 classification, Step 2 coverage, and Step 3 disposition
  satisfy the source idea acceptance criteria.
- Decide whether the source idea is ready for lifecycle close review or whether
  any missed producer-contract packet remains.
- Do not add RV64 lowering, expectation changes, unsupported-marker changes, or
  pass/fail accounting changes in close-readiness review.

## Watchouts

- Do not implement RV64 aggregate lowering for `pr88904` in this plan.
- Do not reinterpret `object #5` as the aggregate payload object; it is the
  hidden pointer parameter object.
- Do not fold inline-asm, clobber, store-publication, select, or
  local-publication residuals into the `sret` producer review.
- Do not infer object layout, alignment, or home ownership in RV64 from source
  or testcase identity.
- Keep expectation, allowlist, unsupported-marker, runtime-comparison, and
  pass/fail accounting changes out of scope.

## Proof

Step 3 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
