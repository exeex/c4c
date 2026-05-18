Status: Active
Source Idea Path: ideas/open/279_aarch64_c_testsuite_llvm_stack_helper_symbols.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Classify Helper-Symbol Failure

# Current Packet

## Just Finished

Step 1 of `plan.md` reproduced and classified the representative
`c_testsuite_aarch64_backend_src_00207_c` failure. The failure is
assembler/link-stage `[BACKEND_FAIL]`, not `[RUNTIME_UNAVAILABLE]`: the backend
route successfully emits AArch64 assembly, then clang reports undefined
references while linking that generated output.

Generated output location:
`build-aarch64-scan/c_testsuite_aarch64_backend/src/00207.c.s`

Evidence in the generated assembly:

- line 5: `bl llvm.stacksave`
- line 7: `bl llvm.dynamic_alloca.i8`
- lines 15 and 18: `bl llvm.stackrestore`

The CTest failure reports the same unresolved helper names from the link step:
`llvm.stacksave`, `llvm.dynamic_alloca.i8`, and `llvm.stackrestore`.

## Suggested Next

Run Step 2: trace where `llvm.stacksave`, `llvm.dynamic_alloca.i8`, and
`llvm.stackrestore` enter the backend route and decide whether ownership belongs
before MIR, in AArch64 lowering, or in symbol/emitter validation.

Suggested narrow proof command for the trace packet:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00207_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not add filename-specific handling for `00207.c`.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Keep completed call-boundary move and `.LBB...` label fixes out of scope
  unless their exact old diagnostics return.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00207_c$'; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected
`c_testsuite_aarch64_backend_src_00207_c` and failed it as `[BACKEND_FAIL]`.
The command exits nonzero because the current backend output leaves LLVM stack
helper symbols unresolved for the assembler/link step. `test_after.log` is the
preserved proof log.
