Status: Active
Source Idea Path: ideas/open/280_aarch64_dynamic_stack_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Trace Dynamic-Stack Records

# Current Packet

## Just Finished

Step 1 of `plan.md` reproduced `c_testsuite_aarch64_backend_src_00207_c` and
traced the prepared dynamic-stack records consumed by AArch64 lowering.

The current focused failure is the expected AArch64 dynamic-stack fail-closed
diagnostic:
`deferred_unsupported: AArch64 dynamic-stack helper lowering is not implemented;
prepared dynamic-stack operations are rejected before unresolved helper calls
reach machine output`.

`test_after.log` contains no unresolved `llvm.stacksave`,
`llvm.dynamic_alloca.*`, `llvm.stackrestore`, or `undefined reference`
diagnostics.

Owner path:

- `src/backend/prealloc/dynamic_stack.cpp`
  `populate_dynamic_stack_plan` recognizes `llvm.stacksave`,
  `llvm.dynamic_alloca.*`, and `llvm.stackrestore` BIR calls and publishes
  `PreparedDynamicStackOp` records.
- `src/backend/prealloc/frame.hpp` defines `PreparedDynamicStackOp` with
  `function_name`, `block_label`, `instruction_index`, `kind`,
  `result_value_name`, `operand_value_name`, `allocation_type_text`,
  `element_size_bytes`, and `element_align_bytes`.
- `src/backend/prealloc/module.hpp` exposes
  `find_prepared_dynamic_stack_plan`.
- `src/backend/mir/aarch64/module/module.hpp` carries
  `FunctionLoweringContext::dynamic_stack_plan`.
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
  `dynamic_stack_helper_kind`, `find_dynamic_stack_op`, and
  `reject_dynamic_stack_helper_call` consume the prepared operation authority
  and currently reject supported helpers before machine output.

Concrete prepared records for `00207.c` function `f1`:

- `dynamic_stack_op block=entry inst_index=1 kind=stack_save result=%t0`
- `dynamic_stack_op block=entry inst_index=4 kind=dynamic_alloca result=%t3 operand=%t2 alloc_type=i8`
- `dynamic_stack_op block=block_3 inst_index=1 kind=stack_restore operand=%t0`
- `dynamic_stack_op block=block_5 inst_index=0 kind=stack_restore operand=%t0`

Related prepared fields visible to implementation:

- `prepared.func @f1 requires_stack_save_restore=yes fixed_slots_use_fp=yes`
- frame plan: `frame_size=16`, `frame_alignment=8`,
  `has_dynamic_stack=yes`, `fixed_slots_use_fp=yes`
- value locations: `%t0` stack-save result in `x20`, `%t2` dynamic allocation
  count in stack slot `#5` at offset `16`, `%t3` dynamic-allocation result in
  `x13`
- call-plan ABI facts: `llvm.stacksave` result comes from call-result `x0` to
  `%t0`/`x20`; `llvm.dynamic_alloca.i8` argument moves from frame slot `#5` to
  ABI `x0` and result from `x0` to `%t3`/`x13`; each `llvm.stackrestore`
  consumes `%t0`/`x20` as ABI argument `x0`.

## Suggested Next

Run Step 2: add a focused backend proof in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` that models
the prepared stack-save, dynamic-alloca, and stack-restore records without
depending on `00207.c`.

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not restore unresolved `llvm.stacksave`, `llvm.dynamic_alloca.*`, or
  `llvm.stackrestore` references in generated AArch64 output.
- Do not weaken expectations, allowlists, unsupported classifications, or stage
  accounting to claim progress.

## Proof

Focused c-testsuite proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00207_c$'; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected
`c_testsuite_aarch64_backend_src_00207_c`; the test failed with the current
AArch64 dynamic-stack unsupported diagnostic. This is expected Step 1
classification evidence, not pass evidence. Runtime-unavailable was not counted
as pass.

Proof log path: `test_after.log`.
