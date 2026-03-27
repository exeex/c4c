# Plan Todo

- Status: Active
- Source Idea: [ideas/open/types_compression_plan.md](/workspaces/c4c/ideas/open/types_compression_plan.md)
- Source plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 6: Compress struct/union body parsing

## Current Slice

- Completed: extracted the record-body member-dispatch setup from
  `parse_struct_or_union()` into a focused helper without changing member
  ordering, template-prelude scope cleanup, or C++ recovery behavior
- New helper path:
  - `try_parse_record_member(...)`
- Added parse-only coverage:
  - `record_member_dispatch_parse`
- Validation completed:
  - focused parser/member regressions passed:
    - `record_member_dispatch_parse`
    - `record_member_method_field_parse`
    - `record_member_special_member_parse`
    - `record_member_prelude_parse`
    - `record_member_enum_parse`
    - `record_member_typedef_using_parse`
    - `record_nested_aggregate_member_parse`
    - `access_labels_parse`
    - `friend_access_parse`
    - `friend_inline_operator_parse`
    - `template_struct_specialization_parse`
    - `constructor_basic`
    - `ctor_init_list_basic`
    - `destructor_member_basic`
    - `operator_extended_member_runtime`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2172 total, 7 failed
    - `test_fail_after.log`: 2173 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting the remaining
  record-body recovery path or another shared follow-up out of
  `parse_struct_or_union()` so the outer loop reads as a thin dispatcher plus
  recovery
- Active target: Step 6 continues with the record-body dispatcher setup in
  `parse_struct_or_union()`; the next slice should extract the remaining
  per-member branch fanout into a focused helper so the main record loop reads
  closer to a pure dispatcher
- This iteration's exact target: add parse-only coverage for a mixed
  record-body sequence that exercises access labels, `friend`,
  `static_assert`, member templates, nested records, enums, special members,
  and regular method/field parsing in one class, then extract the existing
  member-dispatch setup from `parse_struct_or_union()` into a helper without
  changing member ordering, template-prelude scope cleanup, or C++ recovery
  behavior
- Active target: Step 6 continues after the regular method/field extraction;
  the next slice should pull another remaining record-body branch or shared
  method/body finalization path out of `parse_struct_or_union()` so the main
  loop keeps flattening toward a pure dispatcher
- This iteration's exact target: add parse-only coverage for regular in-record
  methods, conversion operators, bitfields, static constexpr field
  initializers, and multi-declarator fields, then extract that existing branch
  into a focused helper without changing operator/member parsing, bitfield
  width capture, static initializer skipping, or field/method insertion order
- Completed: extracted the inline regular method/field branch from
  `parse_struct_or_union()` into a focused helper without changing
  operator/member parsing, bitfield width capture, static initializer
  skipping, or field/method insertion order
- New helper path:
  - `try_parse_record_method_or_field_member(...)`
- Added parse-only coverage:
  - `record_member_method_field_parse`
- Validation completed:
  - focused parser/member regressions passed:
    - `record_member_method_field_parse`
    - `record_member_special_member_parse`
    - `record_member_prelude_parse`
    - `record_member_enum_parse`
    - `record_member_typedef_using_parse`
    - `record_nested_aggregate_member_parse`
    - `access_labels_parse`
    - `friend_access_parse`
    - `friend_inline_operator_parse`
    - `template_struct_specialization_parse`
    - `constructor_basic`
    - `ctor_init_list_basic`
    - `destructor_member_basic`
    - `operator_extended_member_runtime`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2171 total, 7 failed
    - `test_fail_after.log`: 2172 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting one more shared
  method/field follow-up or top-level member-category branch so
  `parse_struct_or_union()` keeps collapsing toward a dispatcher
- Active target: Step 6 continues after the record-body prelude extraction;
  the next slice is constructor/destructor member parsing so
  `parse_struct_or_union()` keeps shrinking toward an explicit top-level
  record-member dispatcher
- This iteration's exact target: add parse-only coverage for in-record
  constructor/destructor members, then extract the current constructor and
  destructor branches into focused helper(s) without changing constructor-name
  matching, initializer-list parsing, deleted/defaulted special-member
  handling, or method insertion order
- Completed: extracted the record-body constructor/destructor branches from
  `parse_struct_or_union()` into focused helpers without changing
  constructor-name matching for specializations, initializer-list parsing,
  deleted/defaulted handling, or method insertion order
- New helper paths:
  - `is_record_special_member_name(...)`
  - `try_parse_record_constructor_member(...)`
  - `try_parse_record_destructor_member(...)`
- Added parse-only coverage:
  - `record_member_special_member_parse`
- Baseline recorded:
  - `test_fail_before.log`: 2170 total, 7 failed
  - failing identities unchanged from the previous iteration
- Validation completed:
  - focused parser regressions passed:
    - `record_member_special_member_parse`
    - `record_member_prelude_parse`
    - `record_member_enum_parse`
    - `record_member_typedef_using_parse`
    - `record_nested_aggregate_member_parse`
    - `access_labels_parse`
    - `friend_access_parse`
    - `friend_inline_operator_parse`
    - `template_struct_specialization_parse`
    - `constructor_basic`
    - `ctor_init_list_basic`
    - `delegating_ctor_basic`
    - `destructor_member_basic`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2170 total, 7 failed
    - `test_fail_after.log`: 2171 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: after constructor/destructor extraction, continue Step 6
  by pulling another top-level method/field dispatcher branch out of
  `parse_struct_or_union()`
- Active target: Step 6 continues with record-body prelude extraction so
  `parse_struct_or_union()` moves toward an explicit top-level dispatcher
- This iteration's exact target: add parse-only coverage for the inlined
  record-body prelude, then extract the current access-label / `friend` /
  `static_assert` / member-template setup path into focused helper(s) without
  changing template-parameter injection, inline-friend body skipping,
  access-label recovery, or `static_assert` semicolon handling
- Active target: Step 6 continues after record enum-member extraction; the
  next slice should keep flattening `parse_struct_or_union()` with another
  top-level member-category helper or prelude helper
- This iteration's exact target: add parse-only coverage for record enum
  members, then extract the existing inline enum-member branch into a focused
  helper without changing enum declarator parsing, bitfield width capture,
  field insertion order, or trailing `;` recovery
- Completed: extracted the record enum-member branch from
  `parse_struct_or_union()` into a focused helper without changing enum
  declarator parsing, bitfield width capture, field insertion order, or
  trailing `;` recovery
- New helper path:
  - `try_parse_record_enum_member(...)`
- Added parse-only coverage:
  - `record_member_enum_parse`
- Validation completed:
  - focused parser regressions passed:
    - `record_member_enum_parse`
    - `record_member_typedef_using_parse`
    - `record_nested_aggregate_member_parse`
    - `access_labels_parse`
    - `eastl_slice7c_struct_body_recovery`
    - `template_struct_specialization_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2168 total, 7 failed
    - `test_fail_after.log`: 2169 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting another top-level
  record-body dispatcher branch or a shared record-member prelude so
  `parse_struct_or_union()` keeps moving toward explicit member-category
  dispatch
- Active target: Step 6 continues with the next top-level record member
  category after typedef/using-member extraction, likely the enum-member path
  or another prelude/helper split that further flattens
  `parse_struct_or_union()`
- This iteration's exact target: extract the record enum-member path or another
  adjacent top-level category helper without changing enum declarator parsing,
  bitfield handling, field insertion order, or access-label recovery
- Completed: extracted the record `using` alias and `typedef` member branches
  from `parse_struct_or_union()` into focused helpers without changing alias
  registration, scoped typedef registration, function-pointer typedef metadata,
  or later member type recognition
- New helper paths:
  - `try_parse_record_using_member(...)`
  - `try_parse_record_typedef_member(...)`
- Added parse-only coverage:
  - `record_member_typedef_using_parse`
- Validation completed:
  - focused parser regressions passed:
    - `record_member_typedef_using_parse`
    - `record_nested_aggregate_member_parse`
    - `access_labels_parse`
    - `eastl_slice7c_struct_body_recovery`
    - `template_struct_specialization_parse`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2167 total, 7 failed
    - `test_after.log`: 2168 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting the record enum-member
  path or another top-level record-body dispatcher branch so
  `parse_struct_or_union()` keeps moving toward explicit member-category
  dispatch
- Active target: Step 6 started; the nested struct/class/union member path is
  now extracted from `parse_struct_or_union()` and the next iteration should
  continue flattening the record-body loop with another top-level member
  category helper
- This iteration's exact target: add parse-only coverage for nested named and
  anonymous aggregate members, then extract the existing nested
  struct/class/union member parsing path into a helper without changing
  declarator handling, anonymous-field synthesis, or field insertion order
- Completed: extracted the nested aggregate member branch from
  `parse_struct_or_union()` into a focused helper without changing nested
  declarator parsing, anonymous-field synthesis, or field insertion order
- New helper path:
  - `try_parse_nested_record_member(...)`
- Added parse-only coverage:
  - `record_nested_aggregate_member_parse`
- Validation completed:
  - focused parser regressions passed:
    - `record_nested_aggregate_member_parse`
    - `access_labels_parse`
    - `eastl_slice7c_struct_body_recovery`
    - `template_struct_specialization_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2166 total, 7 failed
    - `test_fail_after.log`: 2167 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting another top-level
  record-member category helper, likely enum members or typedef/using members,
  so `parse_struct_or_union()` keeps moving toward explicit member-category
  dispatch
- New helper path:
  - `parse_parenthesized_pointer_declarator_inner(...)`
- Added parse-only coverage:
  - extended `declarator_parenthesized_fn_ptr_staging_parse` with anonymous
    parenthesized function-pointer and nested function-pointer parameters
- Validation completed:
  - focused parser regressions passed:
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_pointer_qualifier_staging_parse`
    - `declarator_grouped_suffix_staging_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `declarator_normal_tail_staging_parse`
    - `declarator_array_suffix_staging_parse`
    - `qualified_member_function_pointer_template_owner_parse`
    - `qualified_member_pointer_template_owner_parse`
    - `global_qualified_member_pointer_template_owner_parse`
    - `variadic_param_pack_declarator_parse`
    - `eastl_slice6_template_defaults_and_refqual_cpp`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2166 total, 7 failed
    - `test_after.log`: 2166 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+0` passed, `0` new failures)
- Completed: extracted the shared declarator prefix scan from
  `parse_declarator()` into a helper without changing member-pointer owner
  consumption, `*`/`&`/`&&` staging, nullability skipping, or declarator
  parameter-pack ordering
- New helper path:
  - `parse_declarator_prefix(...)`
- Added parse-only coverage:
  - extended `declarator_pointer_qualifier_staging_parse` with a
    nullability-qualified parameter-pack declarator
- Validation completed:
  - focused parser regressions passed:
    - `declarator_pointer_qualifier_staging_parse`
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_grouped_suffix_staging_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `declarator_normal_tail_staging_parse`
    - `declarator_array_suffix_staging_parse`
    - `qualified_member_function_pointer_template_owner_parse`
    - `qualified_member_pointer_template_owner_parse`
    - `global_qualified_member_pointer_template_owner_parse`
    - `variadic_param_pack_declarator_parse`
    - `eastl_slice6_template_defaults_and_refqual_cpp`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2166 total, 7 failed
    - `test_after.log`: 2166 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+0` passed, `0` new failures)
- Completed: extracted the parenthesized declarator core from
  `parse_declarator()` into a helper without changing pointer-token handling,
  nested function-pointer dispatch, or post-paren array suffix staging
- New helper path:
  - `parse_parenthesized_pointer_declarator(...)`
- Added parse-only coverage:
  - extended `declarator_parenthesized_fn_ptr_staging_parse` with a
    function-returning-pointer-to-array declarator
- Validation completed:
  - focused parser regressions passed:
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_pointer_qualifier_staging_parse`
    - `declarator_grouped_suffix_staging_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `declarator_normal_tail_staging_parse`
    - `declarator_array_suffix_staging_parse`
    - `qualified_member_function_pointer_template_owner_parse`
    - `qualified_member_pointer_template_owner_parse`
    - `global_qualified_member_pointer_template_owner_parse`
    - `variadic_param_pack_declarator_parse`
    - `eastl_slice6_template_defaults_and_refqual_cpp`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2166 total, 7 failed
    - `test_fail_after.log`: 2166 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+0` passed, `0` new failures)
- Completed: extracted the parenthesized pointer-declarator lookahead from
  `parse_declarator()` into a helper without changing attribute skipping,
  pointer-to-member detection, or grouped-vs-parameter-list disambiguation
- New helper path:
  - `is_parenthesized_pointer_declarator_start(...)`
- Added parse-only coverage:
  - extended `declarator_parenthesized_fn_ptr_staging_parse` with an
    attribute-prefixed parenthesized function-pointer declarator
- Validation completed:
  - focused parser regressions passed:
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_pointer_qualifier_staging_parse`
    - `declarator_grouped_suffix_staging_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `qualified_member_function_pointer_template_owner_parse`
    - `qualified_member_pointer_template_owner_parse`
    - `global_qualified_member_pointer_template_owner_parse`
    - `variadic_param_pack_declarator_parse`
    - `eastl_slice6_template_defaults_and_refqual_cpp`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2162 total, 7 failed
    - `test_fail_after.log`: 2166 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+4` passed, `0` new failures)
- Completed: extracted the normal declarator tail from `parse_declarator()`
  into a helper without changing qualified/operator-name capture, attribute
  handling, or array suffix staging order
- New helper path:
  - `parse_normal_declarator_tail(...)`
- Added parse-only coverage:
  - `declarator_normal_tail_staging_parse`
- Validation completed:
  - focused parser regressions passed:
    - `declarator_normal_tail_staging_parse`
    - `declarator_grouped_suffix_staging_parse`
    - `declarator_array_suffix_staging_parse`
    - `declarator_pointer_qualifier_staging_parse`
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `qualified_member_pointer_template_owner_parse`
    - `qualified_member_function_pointer_template_owner_parse`
    - `global_qualified_member_pointer_template_owner_parse`
    - `variadic_param_pack_declarator_parse`
    - `eastl_slice6_template_defaults_and_refqual_cpp`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2165 total, 7 failed
    - `test_after.log`: 2166 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)
- Completed: extracted grouped-declarator lookahead and suffix staging helpers
  from `parse_declarator()` without changing grouped-vs-function-parameter-list
  disambiguation or array suffix ordering
- New helper path:
  - `is_grouped_declarator_start(...)`
  - `try_parse_grouped_declarator(...)`
- Reused the helper path in:
  - grouped declarator lookahead
  - grouped declarator name-plus-array suffix staging
- Added parse-only coverage:
  - `declarator_grouped_suffix_staging_parse`
- Validation completed:
  - focused parser regressions passed:
    - `declarator_grouped_suffix_staging_parse`
    - `declarator_array_suffix_staging_parse`
    - `declarator_pointer_qualifier_staging_parse`
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `global_qualified_member_pointer_template_owner_parse`
    - `qualified_member_function_pointer_template_owner_parse`
    - `qualified_member_pointer_template_owner_parse`
    - `variadic_param_pack_declarator_parse`
    - `eastl_slice6_template_defaults_and_refqual_cpp`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2164 total, 7 failed
    - `test_after.log`: 2165 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)
- Completed: extracted the parenthesized function-pointer suffix coordinator
  step from `parse_declarator()` without changing precedence or
  grouped/parenthesized parsing order
- New helper path:
  - `parse_parenthesized_function_pointer_suffix(...)`
- Reused the helper path in:
  - parenthesized function-pointer declarator suffix parsing
  - nested function-pointer return-parameter storage
  - non-nested function-pointer parameter storage
- Added parse-only coverage:
  - `declarator_member_fn_ptr_suffix_staging_parse`
- Validation completed:
  - focused parser regressions passed:
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_pointer_qualifier_staging_parse`
    - `global_qualified_member_pointer_template_owner_parse`
    - `qualified_member_function_pointer_template_owner_parse`
    - `qualified_member_pointer_template_owner_parse`
    - `variadic_param_pack_declarator_parse`
    - `eastl_slice6_template_defaults_and_refqual_cpp`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2163 total, 7 failed
    - `test_after.log`: 2164 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)

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
- Continued Step 5 by extracting parenthesized function-pointer parameter-list
  and result-storage helpers:
  - `parse_declarator_parameter_list(...)`
  - `store_declarator_function_pointer_params(...)`
- Reduced the `paren_star_peek()` branch in `parse_declarator()` to a more
  helper-driven coordinator around nested and function-returning-function-pointer
  parameter handling
- Added parse-only coverage:
  - `declarator_parenthesized_fn_ptr_staging_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parser regressions passed:
  - `declarator_parenthesized_fn_ptr_staging_parse`
  - `declarator_pointer_qualifier_staging_parse`
  - `declarator_array_suffix_staging_parse`
  - `qualified_member_function_pointer_template_owner_parse`
  - `qualified_member_pointer_template_owner_parse`
  - `variadic_param_pack_declarator_parse`
  - `eastl_slice6_template_defaults_and_refqual_cpp`
- Full clean rebuild `test_fail_after.log` remained monotonic:
  - `test_fail_before.log`: 2162 total, 7 failed
  - `test_fail_after.log`: 2163 total, 7 failed
  - failing tests unchanged
  - regression guard: passed (`+1` passed, `0` new failures)
- Continued Step 5 by extracting grouped declarator lookahead and suffix
  staging helpers:
  - `is_grouped_declarator_start(...)`
  - `try_parse_grouped_declarator(...)`
- Reduced the grouped-declarator branch in `parse_declarator()` to the new
  helper path while keeping grouped-vs-parameter-list disambiguation unchanged
- Added parse-only coverage:
  - `declarator_grouped_suffix_staging_parse`
- Rebuilt successfully with `cmake --build build -j8`
- Targeted parser regressions passed:
  - `declarator_grouped_suffix_staging_parse`
  - `declarator_array_suffix_staging_parse`
  - `declarator_pointer_qualifier_staging_parse`
  - `declarator_parenthesized_fn_ptr_staging_parse`
  - `declarator_member_fn_ptr_suffix_staging_parse`
  - `global_qualified_member_pointer_template_owner_parse`
  - `qualified_member_function_pointer_template_owner_parse`
  - `qualified_member_pointer_template_owner_parse`
  - `variadic_param_pack_declarator_parse`
  - `eastl_slice6_template_defaults_and_refqual_cpp`
- Full clean rebuild `test_after.log` remained monotonic:
  - `test_before.log`: 2164 total, 7 failed
  - `test_after.log`: 2165 total, 7 failed
  - failing identities unchanged
  - regression guard: passed (`+1` passed, `0` new failures)

## Next

- Continue Step 5 with the next smallest declarator suffix extraction from
  `parse_declarator()`, preferably around the remaining parenthesized
  function-pointer branch staging after lookahead is now isolated

## Blockers

- Full `ctest` still reports known pre-existing failures outside the scope of this slice:
  - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
  - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
  - `cpp_positive_sema_template_arg_deduction_cpp`
  - `cpp_positive_sema_template_mixed_params_cpp`
  - `cpp_positive_sema_template_type_subst_cpp`
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
- `parse_declarator()` now delegates parenthesized pointer-declarator lookahead
  to `is_parenthesized_pointer_declarator_start(...)`
- Extended `declarator_parenthesized_fn_ptr_staging_parse` to lock
  attribute-prefixed parenthesized function-pointer declarations before
  refactoring the remaining parenthesized branch
- `parse_parenthesized_pointer_declarator()` now delegates its
  named-vs-nested inner branch to
  `parse_parenthesized_pointer_declarator_inner(...)`
- Extended `declarator_parenthesized_fn_ptr_staging_parse` with anonymous
  parenthesized function-pointer and nested function-pointer parameters to lock
  the extracted branch
- The next intended Step 5 slice is the remaining pointer-token / qualifier /
  name prelude in `parse_parenthesized_pointer_declarator()`
- Current monotonic baseline:
  - `test_fail_before.log`: 2162 total, 7 failed
  - `test_fail_after.log`: 2166 total, 7 failed
- Full-suite failures remain the same known issues; regression guard passed with no new failing tests
