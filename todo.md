Status: Active
Source Idea Path: ideas/open/189_direct_call_no_prototype_variadic_signature_mismatch.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate Signature Construction And Failing Shape

# Current Packet

## Just Finished

Step 1 located the direct-call structured callee signature path and reproduced
the two delegated failures. The construction surface is
`src/codegen/lir/hir_to_lir/call/target.cpp`: `structured_callee_signature`
selects either `lir_call_signature_from_function` for direct `target_fn`
calls or `lir_call_signature_from_fn_ptr_sig` for callable expression
metadata, then `emit_void_call` / `emit_call_with_result` pass that
signature into `make_lir_call_op_with_return_type_ref`. The checking surface
is `src/codegen/lir/verify.cpp`: `verify_call_callee_signature` calls
`parse_lir_typed_call_or_infer_params`, whose structured-signature branch is
in `src/codegen/lir/call_args_ops.hpp`.

Concrete failing shapes:

- No-prototype/old-style direct call: `dr206.c` declares
  `void dr206_unprototyped();` and calls it as `dr206_unprototyped(f)` where
  `f` is `_Complex float`. HIR records the callee as `fn
  dr206_unprototyped() -> void extern`, so direct-function signature
  construction produces `return_type_ref=void`, `fixed_param_types=[]`,
  `fixed_param_type_refs=[]`, `is_variadic=false`,
  `has_unspecified_params=false`, `has_void_param_list=false`. The call
  arguments contain one argument with modeled LLVM type `{ float, float }`.
  The verifier rejects this because a non-variadic structured signature with
  zero fixed parameters requires `args.size() == 0`.
- `va-arg-2.c` immediate reproducer shape: the first visible stop is the
  old-style declaration `extern __SIZE_TYPE__ strlen ();` used as
  `strlen(format)` inside the variadic helpers. HIR records `strlen` as
  `fn strlen() -> ulong extern`; construction therefore emits
  `return_type_ref=i64`, `fixed_param_types=[]`,
  `fixed_param_type_refs=[]`, `is_variadic=false`,
  `has_unspecified_params=false`, `has_void_param_list=false`, while the call
  has one fixed argument modeled as `ptr`. This is the same direct-function
  no-prototype metadata loss as `dr206`, not a backend lowering failure.
- Variadic direct-call shape in the same test: helpers such as
  `f0(char *format, ...)` lower as `return_type_ref=void`,
  `fixed_param_types=["ptr"]`, matching one `ptr` fixed parameter,
  `is_variadic=true`, `has_unspecified_params=false`,
  `has_void_param_list=false`; the call `f0(f+0, 0, ... 15)` has one fixed
  `ptr` argument plus variadic `i32` arguments. A reduced probe of this shape
  compiles, so Step 2 should first repair no-prototype direct-function
  construction before treating later variadic failures as independently
  proven.

Classification: the current mismatch comes from signature construction for
direct `target_fn` calls. HIR/canonical sema tracks unspecified-parameter
function types for old-style declarations, and `FnPtrSig` carries
`unspecified_params`, but `hir::Function` only exposes params plus
`attrs.variadic`; `lir_call_signature_from_function` has no way to set
`has_unspecified_params=true` for `f()` declarations. Argument modeling and
the verifier rule are behaving consistently with the structured values they
receive. The selected Step 2 repair surface is the direct-function signature
construction/resolution boundary in
`src/codegen/lir/hir_to_lir/call/target.cpp`, with likely supporting source
metadata from HIR/canonical function declaration state rather than weakening
`verify_call_callee_signature`.

## Suggested Next

Step 2 should teach direct `target_fn` structured signature construction to
preserve old-style/no-prototype `f()` as `has_unspecified_params=true` while
keeping true `(void)` as `has_void_param_list=true` and keeping variadic fixed
parameter checking intact. Start at `structured_callee_signature` /
`lir_call_signature_from_function`; if `hir::Function` lacks the needed bit,
add the narrow HIR metadata plumbing rather than relaxing the LIR verifier.

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
- Do not classify `va-arg-2.c` as a proven variadic verifier failure until
  the preceding no-prototype `strlen(format)` stop is repaired or bypassed;
  a reduced `f0(char*, ...)` call shape currently passes.

## Proof

Ran exactly:

`ctest --test-dir build --output-on-failure -R '^(clang_c_external_C_drs_dr206_c|llvm_gcc_c_torture_src_va_arg_2_c)$'`

Result: failed as expected, 0% passed, 2/2 tests failed. Both failures are
frontend verifier stops:

- `clang_c_external_C_drs_dr206_c`: `[FRONTEND_FAIL]
  /workspaces/c4c/tests/c/external/clang/C/drs/dr206.c`, `error:
  LirCallOp.callee_signature: structured callee signature does not match call
  arguments`.
- `llvm_gcc_c_torture_src_va_arg_2_c`: `[FRONTEND_FAIL]
  /workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-2.c`, same
  `LirCallOp.callee_signature` verifier error.

No `test_after.log` was written because this delegated packet listed
`test_after.log` under Do Not Touch; the reproducer result above is the proof
record for this investigation packet.
