Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Runtime Route And Host Contract

# Current Packet

## Just Finished

Step 1 audited the AArch64 backend c-testsuite runtime route and host
contract.

- Host proof: `uname -m` reported `aarch64`, so this workspace satisfies the
  native AArch64 runtime path and does not require
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` for runtime execution.
- Native runtime applies when the runner script sees CMake host processor
  `aarch64` or `arm64`; other hosts must provide a runner/emulator through
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.
- Registration is controlled by `ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN` and
  labels tests with `aarch64_backend`; representative smoke cases also receive
  `aarch64_backend_smoke`.
- The cache variable `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is declared in
  `tests/CMakeLists.txt`, passed as `BACKEND_RUNTIME_RUNNER` by
  `tests/c/external/CMakeLists.txt`, and consumed by
  `tests/c/external/c-testsuite-aarch64-backend-runner.cmake`.
- `CODEGEN_MODE=backend-aarch64` is guarded by
  `tests/c/external/c-testsuite-aarch64-backend-runner.cmake` and invokes
  `c4cll --codegen asm --target aarch64-unknown-linux-gnu`.
- Missing runtime on a non-`aarch64`/`arm64` host with an empty
  `BACKEND_RUNTIME_RUNNER` still fails closed as `[RUNTIME_UNAVAILABLE]`.

## Suggested Next

Use this exact Step 2 focused smoke proof command for representative simple
cases `00001.c`, `00002.c`, and `00003.c`:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not exercise the LLVM IR frontend path while claiming AArch64 backend
  runtime coverage.
- Do not weaken `.expected` files, allowlists, unsupported classifications, or
  CTest expectations to claim progress.
- Do not add filename-specific lowering or printer shortcuts for named
  c-testsuite files.

## Proof

Command:

```sh
set -o pipefail; { uname -m; cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan -N -L aarch64_backend; } 2>&1 | tee test_after.log
```

Result: passed with exit code 0. The command printed `aarch64`, configured
`build-aarch64-scan`, found `c4cll` already built, and listed 220
`aarch64_backend` CTest entries. The listed focused smoke representatives were
`c_testsuite_aarch64_backend_src_00001_c`,
`c_testsuite_aarch64_backend_src_00002_c`, and
`c_testsuite_aarch64_backend_src_00003_c`.
