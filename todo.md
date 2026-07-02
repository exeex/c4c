Status: Active
Source Idea Path: ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct The Bucket

# Current Packet

## Just Finished

Lifecycle activation reset `plan.md` and `todo.md` for Step 1 of the RV64
move-bundle target-shape bucket split.

## Suggested Next

Execute Step 1: reconstruct the stable 2026-07-02
`unsupported_move_bundle_target_shape` bucket and prove whether the 183-row
working set is reproducible from current artifacts.

## Watchouts

- Do not implement RV64 lowering or producer repair in this review plan.
- Do not infer ownership from testcase names, target register spellings, or raw
  BIR shape.
- Keep F128-primary rows routed to the existing F128 quarantine lane.
- Stop and report a lifecycle blocker if the bucket cannot be reconciled to the
  source idea's 183-row current fact.

## Proof

Activation proof command:

```sh
git diff --check -- plan.md todo.md
```
