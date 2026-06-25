Status: Active
Source Idea Path: ideas/open/355_rv64_prepared_object_shape_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Scan-Bucket Visibility

# Current Packet

## Just Finished

Step 4 proved scan-bucket visibility for RV64 prepared-object diagnostics
without implementation, test, runner, or expectation edits:

- Ran the delegated full RV64 GCC C torture backend object scan without
  `--dump-prepared-bir`; `test_after.log` records per-case status and links to
  the generated `case.log` files.
- Scan totals were `total=1467 passed=70 failed=1397`.
- `test_after.log` points at actionable representative diagnostic logs:
  `src/20000112-1.c` fails with `module_string_constants`,
  `src/20000224-1.c` fails with `unsupported_global_data`, and
  `src/20001203-1.c` fails with `unsupported_local_memory_access`.
- Generated scan logs also expose broader prepared-object bucket counts:
  `unsupported_instruction_fragment` 347,
  `unsupported_global_data` 234,
  `unsupported_local_memory_access` 142,
  `module_string_constants` 74,
  `unsupported_terminator_fragment` 71,
  `unsupported_param_home` 48,
  `unsupported_stack_frame` 20,
  `unsupported_move_bundle_target_shape` 3,
  `unsupported_function_admission` 2, and
  `prepared_consumer_category` 1.
- Watched representatives `src/20000113-1.c` and `src/20030216-1.c` now pass in
  the scan instead of producing diagnostics.

## Suggested Next

Recommend supervisor close/review for this proof-only runbook slice. If the
source idea remains open after review, the next packet should be a new
supervisor-routed capability packet rather than more diagnostic visibility
work.

## Watchouts

- The scan summary itself does not inline diagnostic text; visibility is via
  the `case.log` paths recorded in `test_after.log`.
- No expectation baseline was changed, and no pass/fail result was reclassified
  to make this proof succeed.
- The scan still has many non-prepared or earlier-pipeline failures such as
  `lir_to_bir lowering before the prepared object handoff`; those are outside
  this proof-only packet.

## Proof

Ran the delegated proof exactly:

```sh
cmake --build --preset default && CASE_TIMEOUT_SEC=20 MAX_CASES=0 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true
```

Result: build passed and the scan completed. `test_after.log` is the preserved
proof log; generated per-case proof logs live under
`build/rv64_gcc_c_torture_backend/*/case.log`.
