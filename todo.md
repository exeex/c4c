Status: Active
Source Idea Path: ideas/open/280_aarch64_dynamic_stack_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove `00207.c` Advances Past The Owned Failure

# Current Packet

## Just Finished

Step 4 of `plan.md` proved that
`c_testsuite_aarch64_backend_src_00207_c` advances beyond the owned AArch64
dynamic-stack unsupported diagnostic.

The focused c-testsuite proof now reaches `[RUNTIME_UNAVAILABLE]`. This is a
runtime-execution blocker only and is not counted as pass evidence.

Generated assembly inspection confirms the dynamic-stack helper calls are gone
and target-owned AArch64 stack behavior is present:

- stack save: `mov x20, sp`;
- dynamic allocation: `ldr x16, [x29, #16]`, `mov x17, sp`,
  `sub x17, x17, x16`, `and x17, x17, #-16`, `mov sp, x17`, and
  `mov x13, sp`;
- stack restore: `mov sp, x20`.

`test_after.log` and
`build-aarch64-scan/c_testsuite_aarch64_backend/src/00207.c.s` contain no
`llvm.stacksave`, `llvm.dynamic_alloca.*`, `llvm.stackrestore`, or old
`AArch64 dynamic-stack helper lowering is not implemented` diagnostic.

## Suggested Next

Run Step 5: execute the broader AArch64 backend scan with `-L aarch64_backend`,
group failures by stage, and confirm that dynamic-stack helper leakage remains
absent across the scan.

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

## Proof

Focused `00207.c` proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00207_c$'; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected
`c_testsuite_aarch64_backend_src_00207_c`; the test failed as
`[RUNTIME_UNAVAILABLE]` because no AArch64 runner is configured. This is not
pass evidence.

Generated assembly inspection:

- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00207.c.s` contains
  stack-save behavior: `mov x20, sp`.
- It contains dynamic allocation behavior: `ldr x16, [x29, #16]`,
  `mov x17, sp`, `sub x17, x17, x16`, `and x17, x17, #-16`,
  `mov sp, x17`, and `mov x13, sp`.
- It contains stack-restore behavior: `mov sp, x20`.
- `test_after.log` and the generated assembly contain no
  `llvm.stacksave`, `llvm.dynamic_alloca.*`, `llvm.stackrestore`, or old
  `AArch64 dynamic-stack helper lowering is not implemented` diagnostics.

Proof log path: `test_after.log`.
