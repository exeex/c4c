Status: Active
Source Idea Path: ideas/open/327_frontend_llvm_indirect_function_pointer_return_type_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Locate The Type Loss

# Current Packet

## Just Finished

Completed Step 1 reproduction and read-only localization for
`c_testsuite_src_00124_c`. The focused failure is reproduced in
`test_after.log`: the generated IR for `main` contains
`%t1 = call i32 (i32, i32) %t0(i32 0, i32 2)`, then tries
`%t2 = load i32, ptr %t1`, so clang rejects `%t1` as `i32` where a pointer is
required. `build/c4cll --dump-hir tests/c/external/c-testsuite/src/00124.c`
also shows the frontend HIR surface has already collapsed `f1`/`p` to
`int*`, while `--dump-canonical` still knows `f1` returns
`ptr(fn(int)(/*unspecified*/))`.

Likely fix site: `src/codegen/lir/hir_to_lir/core.cpp::sig_return_type`.
`Lowerer::fn_ptr_sig_from_decl_node` in `src/frontend/hir/hir_types.cpp`
patches `FnPtrSig::return_type` when declarator metadata says the call returns
a function pointer, but `sig_return_type()` currently bypasses that patched
field whenever `FnPtrSig::canonical_sig` exists and returns
`sema::typespec_from_canonical(*fsig->return_type)` instead. That value is what
`resolve_payload_type(ctx, const CallExpr&)` and
`resolve_call_target_info()` use to set `CallTargetInfo::ret_spec` and
`ret_ty`, producing the first wrong `i32` indirect-call result in
`src/codegen/lir/hir_to_lir/call/target.cpp`.

## Suggested Next

Repair the `FnPtrSig` return-type handoff so canonical-signature lookup does
not discard nested function-pointer return metadata before
`resolve_call_target_info()` derives the LLVM call return type. A focused next
packet should inspect whether `sig_return_type()` can prefer the patched
`sig.return_type.spec` when it carries `is_fn_ptr`/pointer return information,
or whether the canonical-to-TypeSpec conversion itself should preserve that
function-pointer return shape.

## Watchouts

Do not weaken `c_testsuite_src_00124_c` or add named-case special handling for
`00124.c`, `f1`, `f2`, or local variable `p`.

The visible emission site is `src/codegen/lir/hir_to_lir/call/target.cpp`
around `resolve_call_target_info()`: `info.ret_spec =
resolve_payload_type(ctx, call)` followed by `info.ret_ty =
llvm_return_ty(mod_, info.ret_spec)`. The type loss appears to enter through
`src/codegen/lir/hir_to_lir/core.cpp::sig_return_type`, not through
`emit_call_with_result()` itself.

## Proof

Ran the delegated proof:
`set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^c_testsuite_src_00124_c$' --output-on-failure) > test_after.log 2>&1`

Result: failed with the current focused backend/LLVM verification failure.
Proof log: `test_after.log`.
