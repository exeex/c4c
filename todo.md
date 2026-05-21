# Current Packet

Status: Complete
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Fresh Backend Regex Inventory

## Just Finished

Step 1: Capture Fresh Backend Regex Inventory completed a fresh backend-regex
inventory after the 00173-focused closures.

Command run:

```bash
ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log
```

Fresh result summary:

- Selected: 354 tests.
- Passed: 335 tests.
- Failed: 17 tests.
- Timed out: 2 tests.
- CTest summary: `95% tests passed, 19 tests failed out of 354`.
- `c_testsuite_aarch64_backend_src_00173_c`: passed in the fresh result.

Non-passing local backend/unit/CLI tests:

- `backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir` - failed with `[BACKEND_ROUTE_SNIPPET_MISSING]` for `bir.store_local %lv.p, ptr %t0`.

Non-passing external AArch64 c-testsuite backend tests:

- `c_testsuite_aarch64_backend_src_00005_c` - failed with `[FRONTEND_FAIL]`, latest function failure in store local-memory semantic family.
- `c_testsuite_aarch64_backend_src_00112_c` - failed with `[RUNTIME_NONZERO]`, exit 96.
- `c_testsuite_aarch64_backend_src_00119_c` - failed with `[RUNTIME_NONZERO]`, exit 176.
- `c_testsuite_aarch64_backend_src_00140_c` - failed with `[RUNTIME_NONZERO]`, segmentation fault.
- `c_testsuite_aarch64_backend_src_00143_c` - failed with `[RUNTIME_NONZERO]`, exit 1.
- `c_testsuite_aarch64_backend_src_00163_c` - failed with `[RUNTIME_MISMATCH]`.
- `c_testsuite_aarch64_backend_src_00174_c` - failed with `[RUNTIME_MISMATCH]`.
- `c_testsuite_aarch64_backend_src_00176_c` - failed with `[RUNTIME_MISMATCH]`.
- `c_testsuite_aarch64_backend_src_00182_c` - failed with `[RUNTIME_MISMATCH]`.
- `c_testsuite_aarch64_backend_src_00183_c` - failed with `[RUNTIME_MISMATCH]`.
- `c_testsuite_aarch64_backend_src_00187_c` - failed with `[RUNTIME_MISMATCH]`.
- `c_testsuite_aarch64_backend_src_00200_c` - timed out.
- `c_testsuite_aarch64_backend_src_00205_c` - failed with `[RUNTIME_MISMATCH]`.
- `c_testsuite_aarch64_backend_src_00207_c` - timed out.
- `c_testsuite_aarch64_backend_src_00216_c` - failed with `[RUNTIME_NONZERO]`, segmentation fault.
- `c_testsuite_aarch64_backend_src_00217_c` - failed with `[FRONTEND_FAIL]`, latest function failure in load local-memory semantic family.
- `c_testsuite_aarch64_backend_src_00218_c` - failed with `[RUNTIME_MISMATCH]`.
- `c_testsuite_aarch64_backend_src_00204_c` - failed with machine-node printer failure for a deferred unsupported call-boundary move node.

Inventory split:

- Local backend/unit/CLI slice: 142 selected, 141 passed, 1 failed, 0 timed out.
- External `c_testsuite_aarch64_backend_*` slice: 212 selected, 194 passed, 16 failed, 2 timed out.

## Suggested Next

Supervisor should choose the next Step 1 classification packet from the fresh
non-passing inventory. The smallest local-only packet is the single
`backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`
snippet-missing failure; the external c-testsuite packet can be classified
separately from the 18 non-passing AArch64 backend tests.

## Watchouts

- This is an inventory and classification packet, not an implementation
  packet.
- Do not touch implementation files, tests, `ideas/closed/`, `review/`
  artifacts, logs, runner behavior, timeout policy, CTest registration,
  proof-log policy, expectations, or unsupported classifications.
- Do not reactivate stale parked focused ideas from their header alone; use
  current first-bad-fact evidence.
- Fresh evidence keeps the 00173 path passing.
- The delegated proof command uses a pipeline without `pipefail`, so the shell
  command returned through `tee` even though CTest reported non-passing tests;
  use the CTest summary in `test_after.log` as the authoritative inventory
  result.

## Proof

Ran:

```bash
ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log
```

Proof log: `test_after.log`. The command completed and produced the requested
inventory; CTest reported 19 non-passing tests, including 17 failed and 2 timed
out. No code, tests, expectations, runner behavior, timeout policy, CTest
registration, proof-log policy, `plan.md`, or idea files were changed.
