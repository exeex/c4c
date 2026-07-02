Status: Active
Source Idea Path: ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Classification Evidence

# Current Packet

## Just Finished

Step 2 defined the minimum auditable evidence needed to classify each
reconstructed `unsupported_move_bundle_target_shape` row into exactly one
first-owner lane. The rules artifact is
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification_rules.md`.
It defines lane routing for coherent RV64/MIR materialization,
prepared-module target-shape authority gaps, BIR semantic producer gaps,
F128-primary quarantine, and evidence gaps. It also names the future full-row
classification artifact:
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`.

## Suggested Next

Execute Step 3: classify a representative subset of reconstructed rows using
the Step 2 rules before processing the full 183-row bucket.

## Watchouts

- Do not implement RV64 lowering or producer repair in this review plan.
- Do not infer ownership from testcase names, target register spellings, or raw
  BIR shape.
- Keep F128-primary rows routed to the existing F128 quarantine lane.
- The Step 1 table is row identity plus current log path only. Step 3 should
  attach row-level evidence references or derived inspection artifacts before
  assigning ownership.
- Evidence-gap is a valid lane when current artifacts cannot prove first owner
  without guessing.

## Proof

Docs-only proof command, preserved in `test_after.log`:

```sh
git diff --check -- todo.md \
  docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification_rules.md \
  docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_bucket_reconstruction.md
```
