Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove Focused Backend Runtime Smoke Cases

# Current Packet

## Just Finished

Step 2 attempted the focused AArch64 backend runtime smoke proof for
`00001.c`, `00002.c`, and `00003.c`.

- `c_testsuite_aarch64_backend_src_00001_c`: failed at
  `[RUNTIME_UNAVAILABLE]`. Owner layer: AArch64 c-testsuite runtime route/host
  contract, because the script saw an empty host processor and no
  `BACKEND_RUNTIME_RUNNER`.
- `c_testsuite_aarch64_backend_src_00002_c`: failed at
  `[RUNTIME_UNAVAILABLE]`. Owner layer: AArch64 c-testsuite runtime route/host
  contract.
- `c_testsuite_aarch64_backend_src_00003_c`: failed at
  `[RUNTIME_UNAVAILABLE]`. Owner layer: AArch64 c-testsuite runtime route/host
  contract.
- Generated `.s` files exist for all three cases under
  `build-aarch64-scan/c_testsuite_aarch64_backend/src/` and start with
  `.text`, so the runner did not take an LLVM IR fallback path before the
  failure.
- Executables exist for all three cases at the matching `.c.bin` paths, which
  means clang consumed assembler input and completed the link before the
  runtime gate failed.
- Expected sidecars for these three cases are empty. The CTest route did not
  reach runtime expected-output comparison because it stopped at
  `[RUNTIME_UNAVAILABLE]`.

## Suggested Next

Run a narrow Step 3 route fix so `cmake -P` execution gets a real host CPU
classification or an equivalent native-runtime signal, then rerun the same
focused smoke proof.

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not exercise the LLVM IR frontend path while claiming AArch64 backend
  runtime coverage.
- Do not weaken `.expected` files, allowlists, unsupported classifications, or
  CTest expectations to claim progress.
- Do not add filename-specific lowering or printer shortcuts for named
  c-testsuite files.
- Direct native artifact inspection outside the CTest route ran `00001.c.bin`
  and `00002.c.bin` with exit 0 and no output, but `00003.c.bin` exited 1 with
  no output. This is not route pass evidence; it is a likely backend-codegen
  owner-layer issue that the fixed route should expose as `[RUNTIME_NONZERO]`
  after the runtime availability gate is repaired.

## Proof

Command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

Result: failed with exit code 8. Configure and `c4cll` build completed, then
all three selected CTest cases failed at `[RUNTIME_UNAVAILABLE]` after
assembler output and linked binaries had already been produced. Proof log:
`test_after.log`.
