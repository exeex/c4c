Status: Active
Source Idea Path: ideas/open/284_c_aggregate_function_pointer_call_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair ABI Argument Adaptation For Function-Pointer Calls

# Current Packet

## Just Finished

Step 2: repaired the indirect function-pointer call ABI boundary in
`src/codegen/lir/hir_to_lir/expr/coordinator.cpp`. The earlier scalar byte-copy
adaptation was removed because it passed the wrong logical `B` value. Global
function pointers initialized from a concrete function now recover that
function's HIR signature for indirect-call ABI metadata while still calling
through the loaded function pointer. That recovered signature supplies both the
aggregate fixed parameter shape and aggregate return type to the existing call
target and argument preparation paths.

For `llvm_gcc_c_torture_src_struct_ret_1_c`, the indirect
`(*fp)(B1, c2, d3, B2)` call no longer passes `%struct._anon_3` operands where
LLVM expects `i32`; it now uses the same aggregate parameter shape as the direct
call. The indirect call result also no longer has the scalar/aggregate return
mismatch. The generated call now emits:

`%t18 = call %struct._anon_4 (%struct._anon_3, i8, double, %struct._anon_3) %t13(%struct._anon_3 %t14, i8 %t15, double %t16, %struct._anon_3 %t17)`

## Suggested Next

The focused Step 2 proof is green. Suggested next packet: supervisor decides
whether this completed slice should be committed or whether to run broader
aggregate/function-pointer coverage first.

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
- This repair recovers signatures for global function-pointer initializers
  such as `X (*fp)(...) = &f`; local or parameter function pointers whose HIR
  `FnPtrSig` metadata is already scalarized may still need a separate metadata
  repair if similar cases appear.

## Proof

Ran delegated proof and preserved output in `test_after.log`:

`cmake --build build_debug && ctest --test-dir build_debug -R '^llvm_gcc_c_torture_src_struct_ret_1_c$' --output-on-failure`

Result: build succeeded and focused CTest passed.

`100% tests passed, 0 tests failed out of 1`

Supervisor-side broader validation also passed:

`ctest --test-dir build_debug -R '^(frontend_lir_global_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|llvm_gcc_c_torture_src_struct_ret_[12]_c)$' --output-on-failure`

Result: 5/5 passed:
`frontend_lir_global_type_ref`,
`frontend_lir_function_signature_type_ref`,
`frontend_lir_call_type_ref`,
`llvm_gcc_c_torture_src_struct_ret_1_c`,
`llvm_gcc_c_torture_src_struct_ret_2_c`.
