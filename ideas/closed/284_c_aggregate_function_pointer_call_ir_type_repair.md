# 284 C aggregate function-pointer call IR type repair

## Goal

Repair C lowering for aggregate-valued expressions passed through function
pointer calls so generated LLVM IR uses the callee ABI parameter type instead
of passing an aggregate value where a scalar argument is expected.

## Why This Exists

Full `ctest` currently fails
`llvm_gcc_c_torture_src_struct_ret_1_c`. C4CLL emits invalid IR for
`struct-ret-1.c`:

```llvm
%t18 = call i32 (i32, i8, double, i32) %t13(
  i32 %t14, i8 %t15, double %t16, i32 %t17)
```

but `%t14` is an aggregate value:

```llvm
%struct._anon_3 = type { double, [3 x i32], [4 x i8] }
```

This indicates an aggregate return or aggregate argument path is not being
adapted to the function pointer ABI signature before call emission.

## In Scope

- Inspect `tests/c/external/gcc_torture/src/struct-ret-1.c` and the generated
  call lowering around the failing function pointer call.
- Repair the general aggregate-to-call-ABI lowering path for function pointer
  calls.
- Preserve direct aggregate return behavior and existing aggregate call tests.
- Keep the fix semantic; do not special-case the torture testcase.

## Out of Scope

- Do not weaken the gcc torture runtime harness.
- Do not classify the testcase unsupported without explicit approval.
- Do not fold in C++ dependent casts or AArch64 fp128/vararg crash work.

## Acceptance Criteria

- `ctest --test-dir build_debug -R '^llvm_gcc_c_torture_src_struct_ret_1_c$' --output-on-failure`
  passes or advances to a different, explicitly documented non-IR-type
  failure.
- The generated IR no longer passes `%struct._anon_3` as an `i32` argument.
- Nearby aggregate return and function pointer call tests remain green.
- No expectation downgrade, testcase skip, or harness weakening is used.

## Reviewer Reject Signals

- Reject named-case matching for `struct-ret-1.c`.
- Reject changing the torture runner or baseline to hide invalid IR.
- Reject fixes that only alter printed type text while leaving the value/ABI
  mismatch intact.
- Reject broad ABI rewrites without focused proof on the failing aggregate
  function-pointer boundary.

## Closure Note

Closed after the active runbook completed Step 4 and the source idea acceptance
criteria were satisfied.

The implemented repair recovers initialized function-pointer signatures from
the canonical referenced callee/declaration type path, so aggregate return and
fixed aggregate parameter types are lowered consistently for the global
function pointer, concrete function definition, and indirect call. The closure
review found no expectation downgrade, testcase skip, unsupported
classification, harness weakening, or named-case-specific shortcut.

Close-time regression guard used the same five-test subset before and after
the slice:

`ctest --test-dir build_debug -R '^(frontend_lir_global_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|llvm_gcc_c_torture_src_struct_ret_[12]_c)$' --output-on-failure`

Baseline at activation commit `0cda20445` passed 4/5 with
`llvm_gcc_c_torture_src_struct_ret_1_c` failing. Current `main` at
`4f5786084` passed 5/5. `c4c-regression-guard` reported PASS with the focused
torture failure resolved and no new failing tests.
