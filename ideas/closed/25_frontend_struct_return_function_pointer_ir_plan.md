# Struct Return Function-Pointer IR Validation Plan

## Status

Completed on 2026-03-29.

## Summary

Clean rebuild validation during the active x86 regalloc/peephole plan surfaced a separate failure in `llvm_gcc_c_torture_src_struct_ret_1_c`.

The failing case emits invalid LLVM IR for a struct-return function-pointer call in `main`:

- direct call to `@f` uses `call %struct._anon_4 (ptr, i8, double, ptr) @f(...)`
- indirect call through `%t12 = load ptr, ptr @fp` uses `call %struct._anon_4 (%struct._anon_3, i8, double, %struct._anon_3) %t12(...)`
- `clang -x ir` rejects the indirect-call form because the callee type does not match the byval pointer arguments

This is separate from the bounded x86 conditional-return scaffold and should not be silently absorbed into that plan.

## Scope

- identify where function-pointer call lowering drops or rewrites the byval struct parameter types inconsistently
- add the narrowest reproducer around struct-return indirect calls
- make the emitted LLVM IR match Clang’s callable type and argument ABI shape for the supported host triple

## Non-Goals

- unrelated x86 conditional-return work
- general struct ABI cleanup outside the reproducer
- assembler or linker work

## Reproducer

- testcase: `tests/c/external/gcc_torture/src/struct-ret-1.c`
- observed during `ctest --test-dir build -j --output-on-failure` on a clean rebuild on 2026-03-29

## Notes

- the active plan remains `ideas/open/16_backend_x86_regalloc_peephole_enablement_plan.md`
- this idea is inventory only until explicitly activated

## Completion

- added `tests/c/internal/backend_ir_case/struct_return_indirect_byval.c` and the matching `backend_lir_x86_64_struct_return_indirect_byval_ir` test to pin the indirect-call ABI shape
- fixed indirect function-pointer call type-suffix lowering so AMD64 byval aggregate parameters use `ptr` in the callable type, matching the direct-call path
- `llvm_gcc_c_torture_src_struct_ret_1_c` now passes, and the emitted IR for `tests/c/external/gcc_torture/src/struct-ret-1.c` is accepted by `clang -x ir`

## Validation

- targeted checks passed:
  - `backend_lir_x86_64_struct_return_indirect_byval_ir`
  - `llvm_gcc_c_torture_src_struct_ret_1_c`
- full-suite regression logs improved monotonically:
  - `test_before.log`: 26 failing tests
  - `test_after.log`: 25 failing tests
  - removed failure: `llvm_gcc_c_torture_src_struct_ret_1_c`

## Leftover Issues

- the existing x86 backend runtime failures recorded in the regression logs remain outside the scope of this closed idea
