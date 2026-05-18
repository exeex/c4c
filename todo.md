Status: Active
Source Idea Path: ideas/open/280_aarch64_dynamic_stack_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader AArch64 Backend Scan

# Current Packet

## Just Finished

Step 5 of `plan.md` ran the broader AArch64 backend scan with
`-L aarch64_backend` after dynamic-stack lowering.

The scan selected 220 c-testsuite AArch64 backend cases. All 220 are still
classified failures, grouped by stage:

- `[RUNTIME_UNAVAILABLE]`: 190
- `[FRONTEND_FAIL]`: 30
- `[BACKEND_FAIL]`: 0

The broad scan contains no unresolved `llvm.stacksave`,
`llvm.dynamic_alloca.*`, or `llvm.stackrestore` references and no old
`AArch64 dynamic-stack helper lowering is not implemented` diagnostic.

`[RUNTIME_UNAVAILABLE]` remains a runtime proof blocker only. It is not pass
evidence.

## Suggested Next

Run Step 6: completion review for idea `280`, comparing the focused backend
proof, `00207.c` advancement, and broad scan helper-leak absence against the
source acceptance criteria.

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not restore unresolved `llvm.stacksave`, `llvm.dynamic_alloca.*`, or
  `llvm.stackrestore` references in generated AArch64 output.
- Do not weaken expectations, allowlists, unsupported classifications, or stage
  accounting to claim progress.
- Keep missing prepared dynamic-stack operation authority fail-closed before
  machine output.
- The representative `00207.c` path has advanced to runtime-runner
  unavailability; this is not pass evidence.
- Keep the `x17` count-home overlap covered; `x17` is reused for stack-pointer
  computation after preserving the original count.
- The broad scan has zero `[BACKEND_FAIL]` cases and zero dynamic-stack helper
  leakage, but runtime pass evidence is still blocked by missing AArch64
  execution.

## Proof

Broad AArch64 backend scan command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected 220 tests, 0 passed and
220 failed. Stage counts: 190 `[RUNTIME_UNAVAILABLE]`, 30 `[FRONTEND_FAIL]`,
0 `[BACKEND_FAIL]`.

Leakage check: `test_after.log` and generated AArch64 assembly contain no
`llvm.stacksave`, `llvm.dynamic_alloca.*`, `llvm.stackrestore`, or old
`AArch64 dynamic-stack helper lowering is not implemented` diagnostics.

Proof log path: `test_after.log`.
