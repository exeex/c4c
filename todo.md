Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Backend Runtime Proof

# Current Packet

## Just Finished

Completed Step 3: added a focused AArch64 backend runtime proof surface for
the representative c-testsuite cases `src/00001.c`, `src/00002.c`, and
`src/00003.c`.

`tests/c/external/CMakeLists.txt` now appends the additive
`aarch64_backend_smoke` label to those three AArch64 backend registrations
while preserving their existing `c_testsuite` and `aarch64_backend` labels.
The full `aarch64_backend` scan remains intact.

## Suggested Next

Run the same focused smoke proof on a host or CI image with
`C_TESTSUITE_AARCH64_BACKEND_RUNNER` configured to a real AArch64 runtime
runner, then classify the first non-runtime-unavailable failure if one appears.

## Watchouts

- A passing test must use AArch64 backend `.s`, not LLVM IR fallback.
- Runtime-unavailable is not a pass; it is the current host limitation.
- Do not weaken expected outputs or shrink allowlists to get green.
- Do not add named-case AArch64 lowering shortcuts while working on the runner.
- If the focused cases fail after route registration is explicit, classify the
  first failure stage rather than changing expectations: `[FRONTEND_FAIL]`,
  `[BACKEND_OUTPUT_MISSING]`, `[BACKEND_FALLBACK_IR]`, `[BACKEND_FAIL]`,
  `[RUNTIME_UNAVAILABLE]`, `[RUNTIME_NONZERO]`, or `[RUNTIME_MISMATCH]`.

## Proof

Ran the delegated proof command and preserved output in `test_after.log`:

```sh
{ cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend_smoke; } 2>&1 | tee test_after.log
```

Configure and build completed. CTest selected exactly three smoke tests:
`c_testsuite_aarch64_backend_src_00001_c`,
`c_testsuite_aarch64_backend_src_00002_c`, and
`c_testsuite_aarch64_backend_src_00003_c`. Each retained labels
`c_testsuite`, `aarch64_backend`, and `aarch64_backend_smoke`.

Result: `[RUNTIME_UNAVAILABLE]` for all three tests because this host cannot
run the generated AArch64 binaries directly and
`C_TESTSUITE_AARCH64_BACKEND_RUNNER` was not configured.
