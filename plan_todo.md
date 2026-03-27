# Plan Todo

Source plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 4: Consolidate qualified and dependent name consumption

## Current Slice

- Extracted `consume_template_args_followed_by_scope()` for shared `<...>::` consumption
- Reused that helper in pointer-to-member declarator prefixes and qualified declarator-name parsing
- Added a narrow parse-only regression for pointer-to-member owners spelled as template-ids

## Completed

- Read `plan.md`, `README.md`, `AGENT_PROMPT_PLAN.md`, and relevant parser files
- Confirmed `types.cpp` compression frontier still includes:
  - `parse_base_type`
  - `parse_template_argument_list`
  - dependent / qualified name parsing helpers
  - `parse_declarator`
  - `parse_struct_or_union`
- Chose the next smallest safe refactor slice: template argument parsing helper extraction
- Extracted helper methods from `parse_template_argument_list`:
  - `try_parse_template_type_arg(...)`
  - `try_parse_template_non_type_arg(...)`
  - `capture_template_arg_expr(...)`
  - `is_clearly_value_template_arg(...)`
- Reduced `parse_template_argument_list` to a helper-driven coordinator loop
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parse regression checks passed:
  - `template_alias_nttp_expr_inherited_parse`
  - `template_alias_nttp_expr_parse`
  - `template_qualified_nttp_parse`
  - `template_type_context_nttp_parse`
  - `template_typedef_nttp_variants_parse`
  - `template_variadic_qualified_parse`
- Started Step 4 by extracting a reusable qualified/dependent type spelling helper:
  - `consume_qualified_type_spelling(...)`
- Reduced `parse_dependent_typename_specifier(...)` to a helper-driven coordinator
- Reused the same helper in the qualified-name branch of `parse_base_type()`
- Added parse-only coverage:
  - `qualified_dependent_typename_global_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parse regression checks passed:
  - `qualified_dependent_typename_global_parse`
  - `eastl_slice4_typename_and_specialization_parse`
  - `out_of_class_member_owner_scope_parse`
  - `template_qualified_nttp_parse`
  - `template_typedef_nttp_variants_parse`
- Full `ctest` remained monotonic after the slice:
  - `test_before.log`: 2154 total, 3 failed
  - `test_after.log`: 2155 total, 3 failed
  - failing tests unchanged
- Continued Step 4 with a shared `<...>::` helper:
  - `consume_template_args_followed_by_scope()`
- Reused the helper in:
  - pointer-to-member declarator owner parsing
  - qualified declarator-name parsing
- Added parse-only coverage:
  - `qualified_member_pointer_template_owner_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parse regression checks passed:
  - `qualified_member_pointer_template_owner_parse`
  - `member_pointer_param_parse`
  - `eastl_slice7d_qualified_declarator_parse`
  - `out_of_class_member_owner_scope_parse`
- Full `ctest` remained monotonic after the slice:
  - `test_before.log`: 2155 total, 3 failed
  - `test_after.log`: 2156 total, 3 failed
  - failing tests unchanged

## Next

- Continue Step 4 by evaluating whether remaining qualified-name probes can share the extracted helpers without forcing a broader declarator rewrite
- Pick the next smallest slice around parenthesized pointer-to-member-function owners or other residual `<...>::` scans that are still duplicated
- Keep new tests focused on parser-only qualified/dependent spelling coverage before any further behavior changes

## Blockers

- Full `ctest` still reports known pre-existing failures outside the scope of this slice:
  - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`

## Resume Notes

- `plan_todo.md` did not exist before this iteration
- Working tree was clean when this slice started
- `parse_template_argument_list` now uses explicit helpers instead of inline lambdas
- `parse_dependent_typename_specifier` and the qualified-name branch in `parse_base_type` now share `consume_qualified_type_spelling(...)`
- Added `qualified_dependent_typename_global_parse` to lock the shared token-consumption path
- Pointer-to-member owner parsing and qualified declarator-name parsing now share `consume_template_args_followed_by_scope()`
- Added `qualified_member_pointer_template_owner_parse` to lock template-id owner parsing in declarators
- Full-suite failures were treated as existing issues for this iteration per user direction
