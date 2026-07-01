Status: Active
Source Idea Path: ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Focused Materialization Point Inputs

# Current Packet

## Just Finished

Lifecycle switched from completed decomposition idea 482 back to source idea
481. Idea 482 isolated the first focused seam with:

- `tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c`
- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `build/agent_state/482_step4_scalar_compare_destination_probe/decision.md`

The resumed plan must use that focused scalar compare destination probe instead
of auditing the monolithic `930930-1` `%t23` route again.

## Suggested Next

Execute Step 1: audit focused materialization-point inputs. Determine whether
the scalar compare destination probe exposes enough explicit producer evidence
to define a bounded materialization-point contract, or whether another precise
lower-level producer owner still blocks 481.

## Watchouts

- Semantic identity plus destination/layout is not materialization authority.
- Keep downstream interval, frame-slot source fact, branch-stack-load authority,
  and RV64 consumption out of scope.
- Do not infer materialization from raw BIR adjacency, final homes, storage,
  object ids, value names, testcase names, or dump order.
- Do not copy or trim the monolithic `930930-1` route.
- Do not touch `review/`, canonical logs, or baseline files.

## Proof

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed. `test_after.log` was absent, and this lifecycle task was told
not to touch canonical logs, so no new after log was generated.
