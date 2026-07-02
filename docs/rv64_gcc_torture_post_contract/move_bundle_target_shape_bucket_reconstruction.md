# Move-Bundle Target-Shape Bucket Reconstruction

Status: Step 1 reconstructed from current 2026-07-02 evidence.

## Evidence Source

The reconstructed bucket uses the current reset-main/post-cleanup RV64
gcc_torture backend-object scan artifacts documented in
`current_scan_summary.md` and `failure_bucket_map.md`:

- scan date: 2026-07-02
- stable top-level logs:
  - `build/agent_state/rv64_gcc_torture_backend_current_20260702T032151Z.log`
  - `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
- mutable summary:
  - `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- per-case logs:
  - `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

The timestamp pointer currently names
`build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`,
which reports `total=1467 passed=349 failed=1118`.

## Filtering Rule

The row set is derived from the mutable summary and current per-case logs:

```sh
awk -F '\t' 'NR > 1 && $1 == "fail" { print $2 "\t" $3 }' \
  build/agent_state/rv64_gcc_c_torture_backend_summary.tsv |
while IFS="$(printf '\t')" read -r case log; do
  if rg -q 'unsupported_move_bundle_target_shape' "$log"; then
    printf '%s\t%s\n' "$case" "$log"
  fi
done
```

This keeps only failing summary rows whose current `case.log` contains the
explicit `unsupported_move_bundle_target_shape` diagnostic. It does not use
historical move-bundle artifacts or testcase-name inference.

## Reconstructed Artifacts

- durable row table:
  `docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv`
- derived local copy:
  `build/agent_state/unsupported_move_bundle_target_shape_rows.tsv`

The TSV schema is:

```text
case	log
```

The `case` field preserves the gcc_torture source-row identifier from the
summary. The `log` field preserves the current per-case evidence path for
later classification packets.

Step 2 defines the lane evidence contract and future classification TSV schema
in
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification_rules.md`.

## Reconciliation

The reconstructed durable TSV has 183 data rows plus one header row. This
matches the current `unsupported_move_bundle_target_shape` count in
`failure_bucket_map.md`.

Proof command used for this packet:

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
