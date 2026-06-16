Status: Active
Source Idea Path: ideas/open/284_c_aggregate_function_pointer_call_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Nearby Aggregate And Function-Pointer Coverage

# Current Packet

## Just Finished

Step 3: freshly proved the committed Step 2 repair against the focused
`struct-ret-1.c` case and nearby aggregate/function-pointer coverage. The
representative generated IR for `tests/c/external/gcc_torture/src/struct-ret-1.c`
shows the global function pointer, concrete function signature, and indirect
call now agree on aggregate return and aggregate fixed-parameter types:

`@fp = global ptr @f, align 8`

`define %struct._anon_4 @f(%struct._anon_3 %p.a, i8 %p.b, double %p.c, %struct._anon_3 %p.d)`

`%t13 = load ptr, ptr @fp`

`%t18 = call %struct._anon_4 (%struct._anon_3, i8, double, %struct._anon_3) %t13(%struct._anon_3 %t14, i8 %t15, double %t16, %struct._anon_3 %t17)`

## Suggested Next

Step 3 proof is green. Suggested next packet: supervisor decides whether the
runbook needs lifecycle closure, reviewer scrutiny, or another proving slice.

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

Ran exact delegated proof and preserved output in `test_after.log`:

`cmake --build build_debug && ctest --test-dir build_debug -R '^llvm_gcc_c_torture_src_struct_ret_1_c$' --output-on-failure && ctest --test-dir build_debug -R '^(frontend_lir_global_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|llvm_gcc_c_torture_src_struct_ret_[12]_c)$' --output-on-failure`

Result: build succeeded, focused CTest passed, and nearby CTest passed.

`100% tests passed, 0 tests failed out of 1`

`100% tests passed, 0 tests failed out of 5`

Nearby tests passed:
`frontend_lir_global_type_ref`,
`frontend_lir_function_signature_type_ref`,
`frontend_lir_call_type_ref`,
`llvm_gcc_c_torture_src_struct_ret_1_c`,
`llvm_gcc_c_torture_src_struct_ret_2_c`.
