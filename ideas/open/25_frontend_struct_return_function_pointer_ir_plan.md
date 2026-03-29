# Struct Return Function-Pointer IR Validation Plan

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
