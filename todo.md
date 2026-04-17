Status: Active
Source Idea: ideas/open/17_remaining_cpp_positive_cast_and_specialization_member_fixes.md
Source Plan: plan.md

# Current Packet

## Just Finished

- Closed the first investigation loop on the ref-overload direct-call binding
  slice.
- Isolated the remaining failures into two separate lowering problems:
  callable cast/reference initialization and specialized member owner/type
  recovery.
- Patched function-pointer local-init lowering so callable casts stay values
  instead of collapsing into operand addresses or fake reference storage.
- Patched specialized member owner/type recovery so already-realized struct
  tags and field contracts win over stale template-primary AST state.
- Rebuilt the compiler and proved the focused remaining two-test subset passes.
- Re-ran the original 8-case failing batch and confirmed it is fully green.
- Ran broader frontend validation through `frontend_hir_tests` and
  `frontend_parser_tests`.

## Suggested Next

- Commit the remaining-two-failures slice cleanly.
- If you want to continue the original batch beyond this, rerun the wider
  failing subset and open a new idea only for whatever still remains.

## Watchouts

- Do not reopen the already-fixed ref-overload route.
- Do not weaken the two remaining tests.
- Leave unrelated user edits in `AGENTS.md` untouched.

## Proof

- Passed: `cmake --build build -j`
- Passed: `ctest --test-dir build --output-on-failure -R 'cpp_positive_sema_(c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse|template_struct_specialization_runtime)_cpp'`
- Passed: `ctest --test-dir build --output-on-failure -R 'cpp_positive_sema_(c_style_cast_base_ref_qualified_method|c_style_cast_ref_qualified_method|c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse|forwarding_ref_qualified_member_dispatch|function_return_temporary_member_call_runtime|ref_overload_method_basic|ref_overload_method_reads_arg|template_struct_specialization_runtime)_cpp'`
- Passed: `./build/tests/frontend/frontend_hir_tests`
- Passed: `./build/tests/frontend/frontend_parser_tests`
