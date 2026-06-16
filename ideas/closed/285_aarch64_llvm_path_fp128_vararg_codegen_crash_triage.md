# 285 AArch64 LLVM-path fp128/vararg codegen crash triage

## Goal

Triage and repair, or isolate behind an explicit support boundary, the
AArch64 LLVM-path IR that currently crashes Debian clang 19 during object
generation for fp128/long-double and vararg-heavy C torture cases.

## Why This Exists

Full validation currently fails these C runtime cases because C4CLL emits IR
that `clang -x ir` accepts initially but crashes in the AArch64 backend:

- `c_testsuite_src_00204_c`: crash in AArch64 instruction selection for
  `@myprintf`, with `fp128`/long-double HFA/vararg-heavy IR.
- `llvm_gcc_c_torture_src_920625_1_c`: crash in `RegBankSelect` for `@va1`.
- `llvm_gcc_c_torture_src_pr44575_c`: crash in `RegBankSelect` for `@check`.

These are not runtime mismatches; the c2ll binary is never produced.

## In Scope

- Reduce the three crashing IR cases enough to identify the shared unsupported
  construct or separate failure families.
- Determine whether the IR is invalid by LLVM rules, target-incompatible for
  AArch64, or exposing a clang 19 backend bug through avoidable IR shape.
- Repair C4CLL IR generation if it emits invalid or avoidably hostile IR.
- If the issue is an external backend limitation, create an explicit,
  narrowly-scoped support boundary rather than silently skipping tests.

## Out of Scope

- Do not downgrade the tests to unsupported without explicit evidence and
  approval.
- Do not hide clang crashes by suppressing stderr or treating missing binaries
  as runtime no-file failures.
- Do not fold in C++ dependent casts or C aggregate function-pointer ABI work.
- Do not start broad AArch64 ABI rewrites unless reduction proves they are the
  direct cause.

## Acceptance Criteria

- Each of the three failing tests is classified as repaired, reduced to a
  concrete C4CLL IR bug, or parked behind a documented external-backend support
  boundary with evidence.
- Any repaired case passes its focused CTest command.
- The test harness continues reporting clang IR compile failures as
  `[BACKEND_FAIL]` with useful stderr.
- No baseline weakening, testcase skip, or stderr suppression is used as
  progress.

## Reviewer Reject Signals

- Reject broad unsupported downgrades without reduced evidence.
- Reject changes that only mask clang crashes or missing binary outputs.
- Reject named-case-only IR rewriting for the three filenames.
- Reject claims that runtime behavior is fixed when no c2ll binary is produced.
- Reject broad AArch64/vararg/HFA rewrites without focused reductions.

## Closure

Closed 2026-06-16. The three source crash targets were repaired through
semantic AArch64 fp128/HFA vararg lowering changes, without unsupported
classification, expectation weakening, or harness stderr suppression.

Final close proof used:

`cmake --build build_debug && ctest --test-dir build_debug -R '^(frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_global_type_ref|c_testsuite_src_00204_c|llvm_gcc_c_torture_src_920625_1_c|llvm_gcc_c_torture_src_pr44575_c)$' --output-on-failure`

The six selected tests passed, covering the three repaired crash targets and
nearby LIR type-reference checks.
