Status: Active
Source Idea Path: ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct The Bucket

# Current Packet

## Just Finished

Step 1 reconstructed the stable 2026-07-02
`unsupported_move_bundle_target_shape` bucket from
`build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and the current
per-case logs under `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.
The filter keeps failing summary rows whose current `case.log` contains
`unsupported_move_bundle_target_shape`. The durable row table is
`docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv`,
with a matching derived copy at
`build/agent_state/unsupported_move_bundle_target_shape_rows.tsv`.
The reconstructed table has 183 data rows plus one header row.

## Suggested Next

Execute Step 2: define the classification evidence needed to route each
reconstructed row into exactly one first-owner lane.

## Watchouts

- Do not implement RV64 lowering or producer repair in this review plan.
- Do not infer ownership from testcase names, target register spellings, or raw
  BIR shape.
- Keep F128-primary rows routed to the existing F128 quarantine lane.
- The Step 1 table is row identity plus current log path only; later packets
  should cite row-level log evidence before assigning ownership.

## Proof

Docs-only proof command, preserved in `test_after.log`:

```sh
git diff --check -- todo.md \
  docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv \
  docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_bucket_reconstruction.md
printf 'data_rows=%s\n' "$(
  awk -F '\t' 'NR > 1 && $1 == "fail" { print $2 "\t" $3 }' \
    build/agent_state/rv64_gcc_c_torture_backend_summary.tsv |
  while IFS="$(printf '\t')" read -r case log; do
    if rg -q 'unsupported_move_bundle_target_shape' "$log"; then
      printf '%s\t%s\n' "$case" "$log"
    fi
  done | wc -l
)"
test "$(tail -n +2 \
  docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv |
  wc -l)" -eq 183
cmp -s \
  build/agent_state/unsupported_move_bundle_target_shape_rows.tsv \
  docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv
```
