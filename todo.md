Status: Active
Source Idea Path: ideas/open/469_branch_stack_load_authority_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Branch Stack-Load Metadata Inputs

# Current Packet

## Just Finished

Closed idea 451 by split/blocker and activated idea 469 for the precise
producer/prepared branch-stack-load authority metadata prerequisite.

The closed consumer-policy decision:

| Area | Disposition |
| --- | --- |
| Stack-home branch materialization | Not target-consumable from current prepared stack homes alone. |
| Missing owner | Producer/prepared branch-stack-load authority metadata. |
| Needed facts | Branch-site load policy, freshness, clobber safety, scratch/order constraints, and pointer-provenance status. |
| First candidate | `930930-1`, `f.block_1`, `%t2 = ult ptr %t1, %p.reg2`. |
| Separate boundaries | `%t7` pointer-value/provenance and `%t22/%t23` select-result stack-destination remain separate. |

## Suggested Next

Execute Step 1 from `plan.md`: audit branch-stack-load metadata inputs for the
representative rows and classify the first missing prepared metadata field.

Suggested artifact directory:
`build/agent_state/469_step1_branch_stack_load_metadata_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission before available metadata exists.
- Do not weaken GPR-compatible branch predicates.
- Do not infer branch loads, freshness, operands, or conditions from raw BIR,
  stack-slot spelling, block order, filenames, function names, or one prepared
  dump.
- Keep pointer-value/provenance repair and select-result/block-entry
  stack-destination repair separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Lifecycle transition proof:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed.
