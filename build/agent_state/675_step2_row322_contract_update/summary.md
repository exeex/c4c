# 675 Step 2 Row 322 Contract Update

## Classification

Row 322 is classified as a prepared-BIR dump-contract correction, not an
implementation-source change. The current AArch64 prepared-BIR output already
publishes the current-call stack slot and destination facts for `arg index=12`:

```text
arg index=12 value_bank=vreg source_encoding=frame_slot source_value_id=2728
source_slot=#3142 source_stack_offset=8288 source_bank=fpr dest_bank=none
dest_stack_offset=64 dest_stack_size=16
```

The test expectation was stale because it still required
`source_value_id=2732` for the same `source_slot=#3142` and
`dest_stack_offset=64`.

## Evidence

- `build/agent_state/675_post_676_677_residual_reconciliation/summary.md`
  records the Step 1 reconciliation evidence: fresh backend proof left only
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
  failing, and the current dump emitted `source_value_id=2728` for row 322.
- `review/row322_later_lane_review.md` rejects the route that would recover
  `2732` from a later call/store as temporal route drift and testcase-shaped
  evidence rather than a current-call publication rule.
- `ideas/closed/665_aarch64_instruction_dispatch_internal.md` records the
  closure judgment that `%t58.48` / `2728` is the valid current-call source
  identity for `arg index=12`, while `2732` requires the rejected later-call
  lookahead route.

## Change

Updated
`tests/backend/bir/CMakeLists.txt` for
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
so the required prepared-BIR snippet keeps the same current-call stack slot and
destination facts, while correcting only the source identity from stale
`source_value_id=2732` to reviewed current-call `source_value_id=2728`.

No implementation source, baseline logs, allowlists, unsupported markers, or
runtime policy were changed.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed. `test_after.log` records `368/368` backend tests passing,
including
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.
