Status: Active
Source Idea Path: ideas/open/284_c_aggregate_function_pointer_call_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Acceptance Review And Closure Readiness

# Current Packet

## Just Finished

Step 4: completed acceptance-readiness review for the committed aggregate
function-pointer repair. The reviewed diff from `0cda20445..HEAD` names only
`src/codegen/lir/hir_to_lir/expr/coordinator.cpp` and `todo.md`, and repair
commit `33196a3a4` changes only those same files. No test expectations, harness
behavior, or unsupported classifications were weakened.

The repair remains semantic rather than named-case-specific: it recovers the
canonical function signature for initialized function-pointer expressions from
the referenced callee/declaration type path, so aggregate return and fixed
aggregate parameter types are lowered consistently for the global function
pointer, concrete function definition, and indirect call.

## Suggested Next

Step 4 proof is green. Suggested next packet: supervisor hands off to
plan-owner for lifecycle closure readiness, or requests any broader validation
the supervisor wants before closure.

## Watchouts

- Residual risk: this review proves the committed global initialized
  function-pointer repair and nearby LIR type-reference coverage, not every
  possible local or parameter function-pointer metadata path.
- Keep C++ dependent cast work and AArch64 fp128/vararg crash work out of this
  plan.
- Final lifecycle closure remains supervisor/plan-owner owned.

## Proof

Ran exact delegated Step 4 proof and preserved output in `test_after.log`:

`git diff --name-only 0cda20445..HEAD && git show --stat --oneline 33196a3a4 && ctest --test-dir build_debug -R '^(frontend_lir_global_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|llvm_gcc_c_torture_src_struct_ret_[12]_c)$' --output-on-failure`

Result: diff/stat review confirmed no expectations, harness behavior, or
unsupported classifications were weakened; the focused acceptance CTest subset
passed.

`100% tests passed, 0 tests failed out of 5`

Tests passed:
`frontend_lir_global_type_ref`,
`frontend_lir_function_signature_type_ref`,
`frontend_lir_call_type_ref`,
`llvm_gcc_c_torture_src_struct_ret_1_c`,
`llvm_gcc_c_torture_src_struct_ret_2_c`.
