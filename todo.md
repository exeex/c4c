Status: Active
Source Idea Path: ideas/open/284_c_aggregate_function_pointer_call_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate The Aggregate Function-Pointer Boundary

# Current Packet

## Just Finished

Step 1: reproduced `llvm_gcc_c_torture_src_struct_ret_1_c` and located the
first aggregate/function-pointer call ABI divergence. `build_debug/c4cll
--codegen llvm tests/c/external/gcc_torture/src/struct-ret-1.c` emits the direct
call as aggregate-shaped, but the indirect `(*fp)(B1, c2, d3, B2)` call as:

`%t18 = call i32 (i32, i8, double, i32) %t13(i32 %t14, i8 %t15, double %t16, i32 %t17)`

The first bad boundary is the LIR/LLVM call construction boundary where
`resolve_call_target_info`/`llvm_fn_type_suffix_str(FnPtrSig)` render the
function-pointer callee suffix as scalar ABI params, while
`prepare_call_arg` leaves the actual first and fourth argument operands as whole
aggregate loads. The value feeding the bad first `i32` is `%t14 = load
%struct._anon_3, ptr @B1`; the fourth argument has the same shape via `%t17 =
load %struct._anon_3, ptr @B2`. Canonical source typing still shows `fp` as
`ptr(fn(struct _anon_4)(struct _anon_3, char, double, struct _anon_3))`, so the
loss/mismatch is in call ABI rendering/preparation, not parse-time declaration
recognition.

## Suggested Next

Execute Step 2 in `src/codegen/lir/hir_to_lir/call/args.cpp`: make
`StmtEmitter::prepare_call_arg` adapt fixed function-pointer aggregate arguments
to the same ABI shape used by the callee suffix, instead of returning one
`OwnedLirTypedCallArg` with `llvm_value_ty(out_arg_ts)` and the whole aggregate
operand. Candidate helpers to reuse or generalize are
`prepare_amd64_variadic_aggregate_arg`, `amd64_account_type_if_needed`, and the
AMD64 aggregate classifier in `src/codegen/llvm/calling_convention.cpp`.
Also inspect `sig_param_type`/`llvm_fn_type_suffix_str(FnPtrSig)` in
`src/codegen/lir/hir_to_lir/core.cpp`, because `FnPtrSig` accessors prefer
`canonical_sig` over stored HIR params and the observed suffix uses `i32` for
the `B` aggregate slots.

## Watchouts

- Do not special-case `struct-ret-1.c` or generated symbol names.
- Do not weaken the gcc torture harness, expectations, or unsupported
  classification.
- Keep C++ dependent cast work and AArch64 fp128/vararg crash work out of this
  plan.
- The direct `f(B1, c2, d3, B2)` call is not the failing boundary; it emits
  `%struct._anon_3` parameters consistently. The bad case is only the indirect
  function-pointer call where the callee suffix and actual operands are built
  from different ABI authorities.
- `c4c-clang-tool-ccdb` confirmed `sig_param_type` callers in `core.cpp`, but
  did not resolve the split `StmtEmitter::prepare_call_arg` member in
  `call/args.cpp`; use targeted text/compile proof for that file if the AST
  query remains blind there.

## Proof

Ran delegated proof and preserved output in `test_after.log`:

`cmake --build build_debug && ctest --test-dir build_debug -R '^llvm_gcc_c_torture_src_struct_ret_1_c$' --output-on-failure`

Result: build succeeded (`ninja: no work to do`); focused CTest failed with the
expected LLVM verifier/backend failure:

`<stdin>:417:51: error: '%t14' defined with type '%struct._anon_3 = type { double, [3 x i32], [4 x i8] }' but expected 'i32'`
