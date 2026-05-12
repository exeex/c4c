Status: Active
Source Idea Path: ideas/open/189_direct_call_no_prototype_variadic_signature_mismatch.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair No-Prototype And Variadic Semantics

# Current Packet

## Just Finished

Step 2 repaired direct-function structured callee signature metadata for C
old-style/no-prototype declarations. `hir::FnAttr` now carries
`unspecified_params` for C `f()` declarations, `Lowerer::lower_function`
populates it without marking true `(void)` or variadic declarations, and
`lir_call_signature_from_function` maps it to
`LirCallSignature::has_unspecified_params` without inventing fixed parameter
mirrors. The verifier remains strict for prototype and variadic fixed
parameters.

`tests/frontend/frontend_lir_call_type_ref_test.cpp` now covers a direct
`int no_proto(); no_proto(value)` call that must carry
`has_unspecified_params=true`, plus a direct `int no_args(void); no_args()`
call that must keep `has_void_param_list=true` and must not be modeled as
unspecified. Existing direct and indirect variadic checks remain intact.

## Suggested Next

Supervisor should decide whether Step 2 is commit-ready or whether the next
packet should extend this repair to any remaining known no-prototype external
C failures from the parked milestone list.

## Watchouts

- Idea 188 is parked, not closed. Return to its milestone gate after this
  blocker is fixed.
- The 9 known failing tests are:
  `clang_c_external_C_drs_dr206_c`,
  `llvm_gcc_c_torture_src_20010605_2_c`,
  `llvm_gcc_c_torture_src_20051012_1_c`,
  `llvm_gcc_c_torture_src_920501_1_c`,
  `llvm_gcc_c_torture_src_921202_1_c`,
  `llvm_gcc_c_torture_src_921208_2_c`,
  `llvm_gcc_c_torture_src_pr28289_c`,
  `llvm_gcc_c_torture_src_pr34982_c`, and
  `llvm_gcc_c_torture_src_va_arg_2_c`.
- Do not downgrade expectations, suppress signature verification broadly, or
  replace structured metadata with rendered callee text.
- The delegated proof shows `va-arg-2.c` now passes after repairing the
  preceding no-prototype `strlen(format)` direct-call metadata.
- This slice did not add a broad verifier relaxation and did not rewrite
  expectation labels.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(frontend_lir_call_type_ref|clang_c_external_C_drs_dr206_c|llvm_gcc_c_torture_src_20010605_2_c|llvm_gcc_c_torture_src_20051012_1_c|llvm_gcc_c_torture_src_920501_1_c|llvm_gcc_c_torture_src_921202_1_c|llvm_gcc_c_torture_src_921208_2_c|llvm_gcc_c_torture_src_pr28289_c|llvm_gcc_c_torture_src_pr34982_c|llvm_gcc_c_torture_src_va_arg_2_c)$' | tee test_after.log`

Result: passed, 100% tests passed, 10/10 tests passed.

Proof log: `test_after.log`.
