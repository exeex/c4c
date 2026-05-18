Status: Active
Source Idea Path: ideas/open/280_aarch64_dynamic_stack_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Supported AArch64 Dynamic-Stack Lowering

# Current Packet

## Just Finished

Step 3 of `plan.md` implemented and hardened supported AArch64 prepared
dynamic-stack lowering in `src/backend/mir/aarch64/codegen/dispatch.cpp`.

The lowering now consumes prepared `PreparedDynamicStackOp` records before
generic call lowering:

- stack save lowers to target-owned AArch64 output that saves `sp` into the
  prepared result home, such as `mov x20, sp`;
- dynamic alloca lowers by reading the prepared count home, applying printable
  element-size scaling when available, aligning the adjusted stack pointer down
  to 16 bytes, assigning `sp`, and materializing the prepared result home, such
  as `mov x13, sp`;
- when the prepared dynamic-allocation count home overlaps the internal `x17`
  stack-pointer computation scratch, the count is first preserved in `x16`
  before `x17` is reused;
- stack restore lowers to target-owned AArch64 output that restores `sp` from
  the prepared operand home, such as `mov sp, x20`;
- missing homes, unsupported element-size facts, and missing prepared
  dynamic-stack operation authority still fail closed before helper calls reach
  machine output.
- dynamic stack-slot homes without a stable frame-pointer base now fail closed
  with a specific diagnostic instead of emitting `sp`-relative assembly that
  could become invalid after dynamic `sp` changes.

The existing focused backend proof in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` now passes,
including the supported-form no-helper-leak assertion and the unsupported
missing-authority fail-closed assertion. It also covers the `x17` count-home
overlap and the unsafe no-frame-pointer stack-slot home diagnostic.

## Suggested Next

Run Step 4: record the `00207.c` proof/classification result as
`[RUNTIME_UNAVAILABLE]` in the active route, including the generated assembly
inspection that confirms the old dynamic-stack unsupported diagnostic and LLVM
helper references are absent.

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

Backend proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00207_c$'; } 2>&1 | tee test_after.log
```

Result: default build succeeded; `ctest -R '^backend_'` selected 139 backend
tests and all 139 passed, including the new scratch-overlap and unstable
stack-slot focused checks. The AArch64 scan configure/build succeeded;
`c_testsuite_aarch64_backend_src_00207_c` advanced beyond the old dynamic-stack
unsupported diagnostic and failed only as `[RUNTIME_UNAVAILABLE]` because no
AArch64 runner is configured.

Generated assembly inspection:

- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00207.c.s` contains
  stack-save behavior: `mov x20, sp`.
- It contains dynamic allocation behavior: `ldr x16, [x29, #16]`,
  `sub x17, x17, x16`, `and x17, x17, #-16`, `mov sp, x17`, and
  `mov x13, sp`.
- It contains stack-restore behavior: `mov sp, x20`.
- `test_after.log` and the generated assembly contain no
  `llvm.stacksave`, `llvm.dynamic_alloca.*`, `llvm.stackrestore`, or old
  `AArch64 dynamic-stack helper lowering is not implemented` diagnostics.

Proof log path: `test_after.log`.
