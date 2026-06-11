Status: Active
Source Idea Path: ideas/open/190_full_suite_baseline_99_percent_regression_attribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route The Repair Or Baseline Decision

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Route The Repair Or Baseline Decision", by
implementing the focused AArch64 FP same-block global-load repair in
`src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`.

The FP global-load materialization path now still tries Route 3 first and keeps
the existing agreement gate between the Route-3-selected producer instruction
and `PreparedMemoryAccess`. When Route 3 has no usable answer in prepared mode,
it falls back to a prepared same-block `LoadGlobal` producer lookup, mirroring
the integer/publication path, and emits through
`emit_prepared_fp_global_load_to_register`.

Files changed:

- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`
- `todo.md`

## Suggested Next

Supervisor review for acceptance and commit boundary. The narrow repair proof
is green; no executor-side follow-up packet is currently blocked.

## Watchouts

- `PreparedMemoryAccess` remains the target-addressing/address-policy source;
  no BIR target-address payload was introduced.
- The repair is semantic over FP same-block prepared global loads and does not
  match testcase names, global names, or c_testsuite expectations.
- `00040` remains outside this repair packet.

## Proof

Ran the supervisor-delegated proof exactly, with combined output in
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper|c_testsuite_aarch64_backend_src_00119_c|c_testsuite_aarch64_backend_src_00123_c|c_testsuite_aarch64_backend_src_00195_c)$'
```

Result: passed, 5/5 selected tests green:
`backend_aarch64_prepared_memory_operand_records`,
`backend_prepared_lookup_helper`,
`c_testsuite_aarch64_backend_src_00119_c`,
`c_testsuite_aarch64_backend_src_00123_c`, and
`c_testsuite_aarch64_backend_src_00195_c`.

Supervisor regression guard:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: passed; resolved the three c_testsuite failures with no new failures.
