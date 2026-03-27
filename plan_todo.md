# Plan Todo

Source plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 4: Consolidate qualified and dependent name consumption

## Current Slice

- Reuse and centralize qualified/dependent type spelling consumption
- Reduce ad hoc `typename` / `::` / optional template-id token walks
- Keep accepted grammar unchanged unless a focused test proves otherwise

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

## Next

- Start Step 4 by extracting one reusable helper for qualified dependent type spelling
- Make `parse_dependent_typename_specifier` and the qualified-name path in `parse_base_type` share the same token-consumption path where practical
- Add or reuse narrow parse tests around dependent `typename` and qualified template-id consumption before moving code

## Blockers

- Full `ctest` still reports known pre-existing failures outside the scope of this slice:
  - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`

## Resume Notes

- `plan_todo.md` did not exist before this iteration
- Working tree was clean when this slice started
- `parse_template_argument_list` now uses explicit helpers instead of inline lambdas
- Full-suite failures were treated as existing issues for this iteration per user direction
