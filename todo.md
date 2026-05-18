Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Tighten Route Diagnostics If Needed

# Current Packet

## Just Finished

Step 3 fixed the AArch64 c-testsuite backend runner's native-host detection in
`cmake -P` script mode. When `CMAKE_HOST_SYSTEM_PROCESSOR` is empty, the
runner now falls back to `uname -m` before deciding whether native AArch64
execution is available. Non-AArch64 hosts with no runner still fail closed as
`[RUNTIME_UNAVAILABLE]`.

- `c_testsuite_aarch64_backend_src_00001_c`: passed. It went through backend
  `.s` emission, clang `-x assembler` link, executable runtime, and empty
  expected-output comparison.
- `c_testsuite_aarch64_backend_src_00002_c`: passed through the same full
  route and empty expected-output comparison.
- `c_testsuite_aarch64_backend_src_00003_c`: no longer fails at
  `[RUNTIME_UNAVAILABLE]`; it now reaches the real runtime owner layer and
  fails as `[RUNTIME_NONZERO] ... exit=1`.
- Generated `.s` files exist for all three cases under
  `build-aarch64-scan/c_testsuite_aarch64_backend/src/` and start with
  `.text`, so no LLVM IR fallback path was used.
- Executables exist for all three cases at the matching `.c.bin` paths, which
  confirms clang consumed assembler input and produced runtime artifacts.
- Expected sidecars for these three cases are present and empty.

## Suggested Next

Use the now-unblocked smoke route to isolate the `00003.c` backend-codegen
runtime nonzero failure without changing route scripts or expected outputs.

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not exercise the LLVM IR frontend path while claiming AArch64 backend
  runtime coverage.
- Do not weaken `.expected` files, allowlists, unsupported classifications, or
  CTest expectations to claim progress.
- Do not add filename-specific lowering or printer shortcuts for named
  c-testsuite files.
- The `00003.c` generated assembly currently returns without writing the result
  into `w0` (`sub w19, w0, #4; ret`), which matches the observed
  `[RUNTIME_NONZERO] exit=1` and belongs to backend codegen, not the runtime
  route.

## Proof

Command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

Result: failed with exit code 8 because one of three selected tests failed.
Configure and `c4cll` build completed. `00001.c` and `00002.c` passed the full
`.s -> clang -x assembler -> executable -> runtime -> expected-output` route.
`00003.c` failed at `[RUNTIME_NONZERO]` with exit 1. No selected case reported
`[RUNTIME_UNAVAILABLE]`. Proof log: `test_after.log`.
