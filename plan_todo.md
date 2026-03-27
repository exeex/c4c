# Plan Todo

- Status: Active
- Source Idea: [ideas/open/types_compression_plan.md](/workspaces/c4c/ideas/open/types_compression_plan.md)
- Source plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 5: Compress declarator suffix handling

## Current Slice

- Completed: extracted the first Step 5 declarator staging helpers for
  array-suffix parsing/application
- New helper path:
  - `parse_one_declarator_array_dim(...)`
  - `parse_declarator_array_suffixes(...)`
  - `apply_declarator_array_dims(...)`
- Reused the helper path in:
  - parenthesized function-pointer declarators
  - grouped declarators
  - normal declarators
- Added parse-only coverage:
  - `declarator_array_suffix_staging_parse`
- Completed: extracted the next `parse_declarator()` staging helpers around
  pointer/member-pointer/ref qualifier consumption shared by normal and
  parenthesized declarators
- New helper path:
  - `try_parse_declarator_member_pointer_prefix(...)`
  - `apply_declarator_pointer_token(...)`
- Reused the helper path in:
  - normal declarator member-pointer prefix handling
  - normal declarator `*` / `&` / `&&` indirection handling
  - parenthesized function-pointer/member-pointer declarators
- Added parse-only coverage:
  - `declarator_pointer_qualifier_staging_parse`
- Planned validation:
  - add one parse-only case covering grouped / parenthesized declarators with
    pointer qualifiers and ref qualifiers
  - rerun the focused declarator parser cases before the full regression guard

## Completed

- Read `plan.md`, `README.md`, [`prompts/AGENT_PROMPT_EXECUTE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md), and relevant parser files
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
- Continued Step 4 by wiring parenthesized member-function pointer owners
  onto the same qualified/template consumption helpers
- Reused the helper-driven path in:
  - `paren_star_peek()`
  - parenthesized pointer-to-member-function owner consumption
- Added parse-only coverage:
  - `qualified_member_function_pointer_template_owner_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parse regression checks passed:
  - `qualified_member_function_pointer_template_owner_parse`
  - `qualified_member_pointer_template_owner_parse`
  - `member_pointer_param_parse`
  - `eastl_slice5_explicit_decltype_memfn`
  - `eastl_slice7d_qualified_declarator_parse`
- Full `ctest` remained monotonic after the slice:
  - `test_before.log`: 2155 total, 3 failed
  - `test_after.log`: 2157 total, 3 failed
  - failing tests unchanged
- Continued Step 4 by centralizing qualified-name spelling / typedef lookup
  for probe-style parser decisions
- Reused the helper-driven lookup in:
  - `is_type_start()`
  - the qualified-name resolution path inside `parse_base_type()`
- Added parse-only coverage:
  - `qualified_type_start_probe_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parse regression checks passed:
  - `qualified_type_start_probe_parse`
  - `qualified_dependent_typename_global_parse`
  - `using_scoped_typedef_parse`
  - `scope_resolution_expr_parse`
  - `namespace_global_qualifier_parse`
  - `eastl_slice4_typename_and_specialization_parse`
- Full `ctest` remained monotonic after the slice:
  - `test_before.log`: 2157 total, 3 failed
  - `test_after.log`: 2158 total, 3 failed
  - failing tests unchanged
- Continued Step 4 by extracting declarator-side qualified-name helpers:
  - `parse_operator_declarator_name(...)`
  - `parse_qualified_declarator_name(...)`
- Reduced the qualified declarator-name branch in `parse_declarator()` to the new helper path
- Added parse-only coverage:
  - `qualified_operator_template_owner_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parser regressions passed:
  - `qualified_operator_template_owner_parse`
  - `eastl_slice7d_qualified_declarator_parse`
  - `operator_decl_shift_qualified_parse`
  - `qualified_member_function_pointer_template_owner_parse`
  - `qualified_member_pointer_template_owner_parse`
- Full clean rebuild `test_after.log` result:
  - 2159 total, 7 failed
  - added test passed
- Verified the 4 extra clean-build failures are pre-existing, not introduced by this slice:
  - reproduced on detached `HEAD` worktree with a clean build
  - same failing identities on clean `HEAD`:
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Continued Step 4 by centralizing the remaining qualified `::*` owner-prefix consumption
- Extracted helper:
  - `consume_member_pointer_owner_prefix()`
- Reused the helper in:
  - plain pointer-to-member declarators
  - parenthesized pointer-to-member-function lookahead
  - parenthesized pointer-to-member-function consumption
- Added parse-only coverage:
  - `global_qualified_member_pointer_template_owner_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parser regressions passed:
  - `global_qualified_member_pointer_template_owner_parse`
  - `qualified_member_pointer_template_owner_parse`
  - `qualified_member_function_pointer_template_owner_parse`
  - `qualified_operator_template_owner_parse`
  - `eastl_slice7d_qualified_declarator_parse`
  - `member_pointer_param_parse`
  - `out_of_class_member_owner_scope_parse`
- Full clean rebuild `test_after.log` remained monotonic:
  - `test_before.log`: 2159 total, 7 failed
  - `test_after.log`: 2160 total, 7 failed
  - regression guard: passed (`+1` passed, `0` new failures)
- Continued Step 5 by extracting declarator array-suffix staging helpers:
  - `parse_one_declarator_array_dim(...)`
  - `parse_declarator_array_suffixes(...)`
  - `apply_declarator_array_dims(...)`
- Reduced `parse_declarator()` array handling to helper-driven stages in:
  - parenthesized function-pointer declarators
  - grouped declarators
  - normal declarators
- Added parse-only coverage:
  - `declarator_array_suffix_staging_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parser regressions passed:
  - `declarator_array_suffix_staging_parse`
  - `eastl_slice7d_qualified_declarator_parse`
  - `global_qualified_member_pointer_template_owner_parse`
  - `member_pointer_param_parse`
  - `qualified_member_function_pointer_template_owner_parse`
  - `qualified_member_pointer_template_owner_parse`
  - `qualified_operator_template_owner_parse`
  - `variadic_param_pack_declarator_parse`
- Full clean rebuild `test_after.log` remained monotonic:
  - `test_before.log`: 2160 total, 7 failed
  - `test_after.log`: 2161 total, 7 failed
  - regression guard: passed (`+1` passed, `0` new failures)
- Continued Step 5 by extracting shared pointer/member-pointer/ref declarator
  staging helpers:
  - `try_parse_declarator_member_pointer_prefix(...)`
  - `apply_declarator_pointer_token(...)`
- Reduced `parse_declarator()` pointer handling to the new helper path in:
  - normal member-pointer declarator prefixes
  - normal `*` / `&` / `&&` declarator indirection
  - parenthesized function-pointer/member-pointer declarators
- Added parse-only coverage:
  - `declarator_pointer_qualifier_staging_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parser regressions passed:
  - `declarator_pointer_qualifier_staging_parse`
  - `declarator_array_suffix_staging_parse`
  - `eastl_slice6_template_defaults_and_refqual_cpp`
  - `member_pointer_param_parse`
  - `qualified_member_function_pointer_template_owner_parse`
  - `qualified_member_pointer_template_owner_parse`
  - `variadic_param_pack_declarator_parse`
- Full clean rebuild `test_after.log` remained monotonic:
  - `test_before.log`: 2161 total, 7 failed
  - `test_after.log`: 2162 total, 7 failed
  - failing tests unchanged
  - regression guard: passed (`+1` passed, `0` new failures)

## Next

- After pointer/ref qualifier staging, extract the next smallest
  parenthesized-function-pointer coordinator step without changing declarator
  precedence

## Blockers

- Full `ctest` still reports known pre-existing failures outside the scope of this slice:
  - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
- Clean rebuilds also currently expose pre-existing runtime failures that are
  reproducible on detached `HEAD` without this patch:
  - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
  - `cpp_positive_sema_template_arg_deduction_cpp`
  - `cpp_positive_sema_template_mixed_params_cpp`
  - `cpp_positive_sema_template_type_subst_cpp`
- Deferred follow-on note from test shaping:
  - top-level reference-to-function-pointer declarators like
    `int (*const& ref)();` still fail parse, while the same spelling parses in
    local-declaration and parameter contexts; not required for this Step 5
    compression slice

## Resume Notes

- `plan_todo.md` did not exist before this iteration
- Working tree was clean when this slice started
- `parse_template_argument_list` now uses explicit helpers instead of inline lambdas
- `parse_dependent_typename_specifier` and the qualified-name branch in `parse_base_type` now share `consume_qualified_type_spelling(...)`
- Added `qualified_dependent_typename_global_parse` to lock the shared token-consumption path
- Pointer-to-member owner parsing and qualified declarator-name parsing now share `consume_template_args_followed_by_scope()`
- Added `qualified_member_pointer_template_owner_parse` to lock template-id owner parsing in declarators
- Parenthesized member-function pointer owners now reuse the same helper path in both lookahead and real consumption
- Added `qualified_member_function_pointer_template_owner_parse` to lock that declarator form
- `is_type_start()` and the qualified-name branch in `parse_base_type()` now share helper-based qualified-name spelling / typedef lookup
- Added `qualified_type_start_probe_parse` to lock declaration-context probing for qualified type names
- The next intended extraction is the declarator-side qualified-name loop used for out-of-class names and operator declarators
- `parse_declarator()` now delegates qualified/operator declarator-name parsing to dedicated helpers
- Added `qualified_operator_template_owner_parse` to lock templated owner spellings for out-of-class operator declarators
- The remaining `::*` owner-prefix duplication now lives behind `consume_member_pointer_owner_prefix()`
- Added `global_qualified_member_pointer_template_owner_parse` to lock global-qualified templated owner spellings through the shared helper path
- Step 4 is sufficiently consolidated for now; the next work is Step 5 helper
  extraction inside `parse_declarator()`
- The immediate Step 5 slice is array-suffix staging for grouped, normal, and
  parenthesized function-pointer declarators
- `parse_declarator()` now uses dedicated helper methods for array-dimension
  parsing/application across all three declarator paths
- Added `declarator_array_suffix_staging_parse` to lock grouped array and
  function-pointer-array suffix handling
- This iteration is targeting the next Step 5 extraction around shared
  pointer/member-pointer/ref qualifier consumption before array/function
  suffixes
- `parse_declarator()` now shares helper-based pointer/member-pointer/ref token
  application across normal and parenthesized declarators
- Added `declarator_pointer_qualifier_staging_parse` to lock grouped
  function-pointer refs and member-function-pointer qualifiers in local-decl
  form
- Current monotonic baseline:
  - `test_before.log`: 2161 total, 7 failed
  - `test_after.log`: 2162 total, 7 failed
- Full-suite failures remain the same known issues; regression guard passed with no new failing tests
