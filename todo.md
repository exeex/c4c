Status: Active
Source Idea Path: ideas/open/421_rv64_instruction_fragment_bucket_classification.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Write Follow-Up Ideas For Implementation-Ready Buckets

# Current Packet

## Just Finished

Step 4 created durable open follow-up ideas from the Step 2/3
`unsupported_instruction_fragment` evidence:

- `ideas/open/427_rv64_scalar_select_join_materialization.md`
- `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
- `ideas/open/429_rv64_pointer_address_materialization.md`
- `ideas/open/430_rv64_integer_div_rem_lowering.md`
- `ideas/open/431_prepared_aggregate_abi_contract_review.md`

The ideas cover the top three ordinary non-F128 candidates plus integer
div/rem and prepared aggregate ABI review. Each idea cites the bucket evidence,
names the owning layer, and includes concrete reviewer reject signals for
testcase-shaped shortcuts, expectation downgrades, producer-gap bypasses, and
scope mixing.

## Suggested Next

Execute Step 5 from `plan.md`: validate the lifecycle/docs slice with
`git diff --check`, summarize completed buckets, generated ideas, postponed
F128 handling, and any unresolved evidence gaps, then let the supervisor decide
whether the source idea is complete or needs another runbook.

## Watchouts

- Do not implement RV64 lowering while validating this classification runbook.
- Keep F128 quarantined and lowest priority unless future evidence proves it
  blocks a broader ordinary non-F128 owner.
- Keep producer-owned work separate: aggregate `sret`/`byval` review is in
  `ideas/open/431_prepared_aggregate_abi_contract_review.md`, and pointer rows
  with incomplete provenance must remain outside
  `ideas/open/429_rv64_pointer_address_materialization.md`.
- Step 5 should not invent broad code validation for this docs/lifecycle-only
  runbook unless the supervisor delegates it.

## Proof

Pending for Step 5: `git diff --check`.
