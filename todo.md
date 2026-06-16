Status: Active
Source Idea Path: ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Avoidable Or Invalid IR Shapes

# Current Packet

## Just Finished

Step 3 repaired the residual AArch64 HFA stdarg runtime mismatch at the
caller-side variadic argument boundary. Clang keeps variadic HFAs as aggregate
array carriers such as `[4 x double] alignstack(8)` and `[4 x fp128]
alignstack(16)`, while the existing C4C path flattened every variadic HFA into
independent scalar FP operands. That was correct for fixed HFA parameters but
wrong for variadic overflow: once the FP register-save area was exhausted, LLVM
laid out the spilled scalar operands as separate call slots while
`emit_aarch64_vaarg_hfa` correctly copied a compact aggregate stack payload.

`src/codegen/lir/hir_to_lir/call/args.cpp` now preserves fixed AArch64 HFA lane
flattening but emits non-Apple AArch64 variadic HFAs as LLVM aggregate array
carriers with `alignstack`. The callee-side explicit `emit_aarch64_vaarg_hfa`
lane extraction and overflow accounting were left intact. No expectation files,
unsupported classifications, `plan.md`, or source ideas were touched.

## Suggested Next

Supervisor should review and commit the completed Step 3 slice, then decide
whether the active runbook needs broader AArch64 variadic validation or plan
lifecycle review.

## Watchouts

- The semantic boundary is fixed-vs-variadic AArch64 HFA call lowering: fixed
  formal parameters still require scalar lane flattening, but variadic HFAs
  must remain aggregate carriers so LLVM can choose register-save versus
  compact overflow layout coherently.
- The focused proof covers the prior fp128 crash target plus the two repaired
  non-fp128 passes. Broader AArch64 variadic coverage remains a supervisor
  validation-policy decision.
- The generated 00204 IR now matches Clang's key variadic HFA surface shape for
  long double, double, and float aggregate carriers.

## Proof

Ran:

`(cmake --build build_debug && ctest --test-dir build_debug -R '^(c_testsuite_src_00204_c|llvm_gcc_c_torture_src_920625_1_c|llvm_gcc_c_torture_src_pr44575_c)$' --output-on-failure) 2>&1 | tee test_after.log`

Result: build succeeded and all three focused targets passed:
`c_testsuite_src_00204_c`, `llvm_gcc_c_torture_src_920625_1_c`, and
`llvm_gcc_c_torture_src_pr44575_c`. Proof log: `test_after.log`.

Additional non-canonical inspection:

`build_debug/c4cll --target aarch64-linux-gnu --codegen llvm -o /tmp/c4c_00204_after.ll tests/c/external/c-testsuite/src/00204.c && rg -n "alignstack|\\[4 x fp128\\]|\\[4 x double\\]|\\[4 x float\\]" /tmp/c4c_00204_after.ll`

Result: 00204 variadic HFA call sites now use aggregate array carriers with
`alignstack` rather than scalar lane argument lists.

Supervisor-side broader validation:

`ctest --test-dir build_debug -R '^(frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_global_type_ref|c_testsuite_src_00204_c|llvm_gcc_c_torture_src_920625_1_c|llvm_gcc_c_torture_src_pr44575_c)$' --output-on-failure`

Result: 6/6 passed: `frontend_lir_global_type_ref`,
`frontend_lir_function_signature_type_ref`, `frontend_lir_call_type_ref`,
`c_testsuite_src_00204_c`, `llvm_gcc_c_torture_src_920625_1_c`, and
`llvm_gcc_c_torture_src_pr44575_c`.
