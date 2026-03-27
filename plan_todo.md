# Plan Todo

- Status: Active
- Source Idea: [ideas/open/types_compression_plan.md](/workspaces/c4c/ideas/open/types_compression_plan.md)
- Source plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 4: Consolidate Qualified And Dependent Name Consumption

## Current Slice

- Completed: extracted the remaining global-qualified declaration-context
  probe from `is_type_start()` into a shared qualified-type-start helper so
  unresolved `::ns::Type` namespace types now reuse the same fallback
  classification as `try_parse_qualified_base_type(...)` without
  misclassifying neighboring qualified calls
- New helper path:
  - `can_start_qualified_type_declaration(...)`
- Added parse-only coverage:
  - `qualified_global_type_start_shared_probe_parse`
- Validation completed:
  - focused qualified/dependent-name regressions passed:
    - `qualified_global_type_start_shared_probe_parse`
    - `qualified_cpp_base_type_dispatch_parse`
    - `qualified_type_resolution_dispatch_parse`
    - `qualified_type_start_probe_parse`
    - `qualified_type_start_shared_probe_parse`
    - `qualified_type_spelling_shared_parse`
    - `qualified_dependent_typename_global_parse`
    - `template_member_type_direct_parse`
    - `template_member_type_inherited_parse`
    - `qualified_member_pointer_template_owner_parse`
    - `qualified_member_function_pointer_template_owner_parse`
    - `qualified_operator_template_owner_parse`
    - `global_qualified_member_pointer_template_owner_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2196 total, 7 failed
    - `test_fail_after.log`: 2197 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 4 is likely near complete after the global-qualified
  probe extraction; the next slice should explicitly inspect whether any
  qualified/dependent-name duplication remains, otherwise advance to Step 5
  declarator suffix handling
- This iteration's exact target: completed the bounded Step 4 extraction
  around global-qualified declaration-context probing in `is_type_start()` and
  verified it against nearby parser coverage plus the full suite
- Next intended slice: audit the remaining Step 4 paths for any duplicated
  qualified/dependent-name classification not already routed through the shared
  probe/helper set; if none remain, move the active item to Step 5
- Completed: extracted the remaining C++ scoped/dependent base-type dispatch
  from `parse_base_type()` into `try_parse_cpp_scoped_base_type(...)` so the
  coordinator now funnels leading `typename` dependent types and
  global-/namespace-qualified typedef-style names through one helper without
  changing qualified lookup order or token consumption
- New helper path:
  - `try_parse_cpp_scoped_base_type(...)`
- Added parse-only coverage:
  - `qualified_cpp_base_type_dispatch_parse`
- Validation completed:
  - focused qualified/dependent-name regressions passed:
    - `qualified_cpp_base_type_dispatch_parse`
    - `qualified_type_start_shared_probe_parse`
    - `qualified_type_resolution_dispatch_parse`
    - `qualified_type_spelling_shared_parse`
    - `qualified_dependent_typename_global_parse`
    - `template_member_type_direct_parse`
    - `template_member_type_inherited_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2195 total, 7 failed
    - `test_fail_after.log`: 2196 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 4 remains active after the scoped/dependent dispatcher
  extraction; the next slice should centralize one more bounded qualified-name
  classification/helper path still split between `is_type_start()` probing and
  `parse_base_type()` dispatch
- This iteration's exact target: completed the bounded Step 4 extraction around
  the remaining inline C++ scoped/dependent base-type dispatch in
  `parse_base_type()` and verified it against nearby parser coverage plus the
  full suite
- Next intended slice: inspect the remaining shared qualified-name
  classification branch between `is_type_start()` and `parse_base_type()`, then
  extract one focused helper without broadening accepted grammar
- Active target: Step 4 remains active; this iteration is focused on the
  remaining C++ scoped/dependent base-type dispatch still inline in
  `parse_base_type()`
- This iteration's exact target: add one focused parse-only regression around
  `parse_base_type()` consuming both leading `typename` dependent types and
  global-qualified typedef-style names, then extract that bounded dispatch into
  one helper without changing token consumption or typedef lookup order
- Next intended slice: if this dispatcher extraction stays monotonic, centralize
  one more bounded qualified-name classification helper that is still split
  between `is_type_start()` and `parse_base_type()`
- Baseline recorded:
  - `test_fail_before.log`: 2195 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Completed: extracted the shared qualified-type-start classification probe
  from the remaining Step 4 overlap so `is_type_start()` and
  `try_parse_qualified_base_type(...)` now reuse the same qualified-name
  typedef-resolution / unresolved-qualified fallback metadata without
  changing token consumption or the declaration-vs-expression heuristic
- New helper path:
  - `probe_qualified_type(...)`
- Added parse-only coverage:
  - `qualified_type_start_shared_probe_parse`
- Validation completed:
  - focused qualified/dependent-name regressions passed:
    - `qualified_type_start_shared_probe_parse`
    - `qualified_type_start_probe_parse`
    - `qualified_type_resolution_dispatch_parse`
    - `qualified_type_spelling_shared_parse`
    - `qualified_dependent_typename_global_parse`
    - `template_member_type_direct_parse`
    - `template_member_type_inherited_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2194 total, 7 failed
    - `test_fail_after.log`: 2195 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 4 remains active; this iteration is focused on the
  remaining overlap between `is_type_start()` qualified-type probing and
  `parse_base_type()` qualified dispatch
- This iteration's exact target: extract one shared qualified-type-start
  predicate so declaration-context probing reuses the same qualified-name
  classification rules as `try_parse_qualified_base_type(...)` without
  changing token consumption or accepted grammar
- Next intended slice: Step 4 remains active after the shared probe
  extraction; centralize one more bounded qualified/dependent-name
  classification branch still inline in `parse_base_type()`
- Baseline recorded:
  - `test_fail_before.log`: 2194 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Completed: extracted the remaining `peek_qualified_name(...)`
  classification path from `parse_base_type()` into
  `try_parse_qualified_base_type(...)` so the coordinator now delegates the
  resolved-qualified-typedef vs unresolved-qualified-type fallback decision
  through one helper without changing lookup order or qualified spelling
  consumption
- New helper path:
  - `try_parse_qualified_base_type(...)`
- Added parse-only coverage:
  - `qualified_type_resolution_dispatch_parse`
- Validation completed:
  - focused qualified/dependent-name regressions passed:
    - `qualified_type_resolution_dispatch_parse`
    - `qualified_type_spelling_shared_parse`
    - `qualified_dependent_typename_global_parse`
    - `qualified_type_start_probe_parse`
    - `template_member_type_direct_parse`
    - `template_member_type_inherited_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2193 total, 7 failed
    - `test_fail_after.log`: 2194 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 4 remains active after the qualified-name
  classification extraction; the next slice should centralize more of the
  remaining qualified/dependent-name consumption logic that is still split
  between `is_type_start()` probing and `parse_base_type()` dispatch
- This iteration's exact target: completed the bounded Step 4 helper
  extraction around qualified-name classification in `parse_base_type()` and
  verified it against nearby parser coverage plus the full suite
- Next intended slice: inspect the remaining overlap between
  `is_type_start()` qualified-type probing and `parse_base_type()` qualified
  dispatch, then extract one focused helper or shared predicate without
  broadening accepted grammar
- Completed: extracted the shared qualified-type spelling entry from the
  remaining Step 4 path into
  `consume_qualified_type_spelling_with_typename(...)` so
  `parse_dependent_typename_specifier()` and the direct qualified-type branch
  in `parse_base_type()` now consume their final spelling through the same
  token-walk helper without changing typedef resolution or qualified fallback
  policy
- New helper path:
  - `consume_qualified_type_spelling_with_typename(...)`
- Added parse-only coverage:
  - `qualified_type_spelling_shared_parse`
- Validation completed:
  - focused qualified/dependent-name regressions passed:
    - `qualified_type_spelling_shared_parse`
    - `qualified_dependent_typename_global_parse`
    - `qualified_type_start_probe_parse`
    - `template_member_type_direct_parse`
    - `template_member_type_inherited_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2192 total, 7 failed
    - `test_fail_after.log`: 2193 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 4 remains active after the shared qualified-type
  spelling extraction; the next slice should isolate more of the
  classification/resolution logic that still sits inline in
  `parse_base_type()`
- This iteration's exact target: completed the bounded Step 4 extraction
  around the shared qualified-type spelling token walk and verified it against
  nearby parser coverage plus the full suite
- Next intended slice: extract one focused helper for the remaining
  `peek_qualified_name(...)` classification path inside `parse_base_type()`,
  likely the resolved-qualified-typedef vs unresolved-qualified-type decision,
  without changing name lookup order
- Baseline recorded:
  - `test_fail_before.log`: 2192 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Completed: extracted the remaining expression-parse vs template-close
  acceptance fallback from `try_parse_template_non_type_arg()` into
  `try_parse_template_non_type_expr(...)` so the non-type path now reads as
  literal / identifier / parsed-expression / token-capture staging without
  changing the existing ambiguous-case acceptance order
- New helper path:
  - `try_parse_template_non_type_expr(...)`
- Added parse-only coverage:
  - `template_argument_expr_close_staging_parse`
- Baseline recorded:
  - `test_fail_before.log`: 2191 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused template-argument regressions passed:
    - `template_argument_expr_close_staging_parse`
    - `template_argument_loop_staging_parse`
    - `template_alias_nttp_expr_parse`
    - `template_typedef_nttp_variants_parse`
    - `template_typename_typed_nttp_parse`
  - focused known blocker unchanged:
    - `eastl_probe_pack_expansion_template_arg_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2191 total, 7 failed
    - `test_fail_after.log`: 2192 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 3 is complete relative to the plan’s helper-driven
  template-argument disambiguation bar; Step 4 is now the next incomplete
  compression cluster in `types.cpp`
- This iteration's exact target: completed the bounded Step 3 extraction
  around expression-style non-type template arguments that terminate directly
  at the template close and verified it against nearby parser coverage plus
  the full suite
- Next intended slice: start Step 4 with a focused parse-only case around
  qualified dependent type spelling, then extract one reusable helper from the
  remaining ad hoc qualified-name/template-arg consumption path
- Completed: extracted the per-argument disambiguation and comma-progression
  step from `parse_template_argument_list()` into
  `parse_next_template_argument(...)` so the outer loop now reads as a
  `<...>` coordinator without changing the existing type-vs-non-type
  acceptance order
- New helper path:
  - `parse_next_template_argument(...)`
- Added parse-only coverage:
  - `template_argument_loop_staging_parse`
- Baseline recorded:
  - `test_before.log`: 2190 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused template-argument regressions passed:
    - `template_argument_loop_staging_parse`
    - `template_alias_nttp_expr_parse`
    - `template_typedef_nttp_variants_parse`
    - `template_typename_typed_nttp_parse`
  - focused known blocker unchanged:
    - `eastl_probe_pack_expansion_template_arg_parse`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2190 total, 7 failed
    - `test_after.log`: 2191 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 3 remains active after the per-argument loop extraction;
  the next slice should isolate the remaining expression-capture vs
  template-close fallback inside `try_parse_template_non_type_arg()` without
  changing accepted ambiguous cases
- This iteration's exact target: completed the first bounded Step 3 helper
  extraction around the template-argument loop skeleton and verified it against
  nearby parser coverage plus the full suite
- Next intended slice: add a focused parse-only case for an expression-style
  non-type template argument near a template close, then extract the remaining
  expression-capture fallback from `try_parse_template_non_type_arg()` into one
  helper
- Completed: refreshed the Step 5 baseline and then extracted the remaining
  post-`RParen` parenthesized-pointer suffix/finalization flow from
  `parse_parenthesized_pointer_declarator()` into
  `finalize_parenthesized_pointer_declarator(...)`, leaving the outer function
  as prefix / inner / finalization coordination without changing function
  suffix parsing, array suffix parsing, or `is_ptr_to_array` behavior
- New helper path:
  - `finalize_parenthesized_pointer_declarator(...)`
- Added parse-only coverage:
  - `declarator_parenthesized_member_pointer_finalization_parse`
- Baseline recorded:
  - `test_fail_before.log`: 2189 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused declarator regressions passed:
    - `declarator_parenthesized_member_pointer_finalization_parse`
    - `declarator_parenthesized_pointer_inner_staging_parse`
    - `declarator_parenthesized_member_pointer_prefix_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_array_suffix_staging_parse`
    - `declarator_normal_tail_staging_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2189 total, 7 failed
    - `test_fail_after.log`: 2190 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 5 is complete relative to the plan’s declarator
  coordinator bar; Step 7 now tracks regression containment while the next
  incomplete compression cluster in `types.cpp` is selected
- This iteration's exact target: completed the final bounded Step 5 helper
  extraction around parenthesized-pointer finalization and verified it against
  the nearby declarator slice plus the full suite
- Next intended slice: choose the highest-priority remaining incomplete
  compression cluster in `types.cpp` and start it with a new focused parse
  test before implementation
- Completed: extracted the remaining inner branch from
  `parse_parenthesized_pointer_declarator_inner()` into focused helpers so
  the function now reads as array-skip / name-capture / nested-recursion
  coordination without changing parenthesized pointer or member-pointer
  suffix behavior
- New helper paths:
  - `skip_parenthesized_pointer_declarator_array_chunks(...)`
  - `parse_parenthesized_pointer_declarator_name(...)`
  - `try_parse_nested_parenthesized_pointer_declarator(...)`
- Added parse-only coverage:
  - `declarator_parenthesized_pointer_inner_staging_parse`
- Baseline recorded:
  - `test_fail_before.log`: 2188 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused declarator regressions passed:
    - `declarator_parenthesized_pointer_inner_staging_parse`
    - `declarator_parenthesized_member_pointer_prefix_parse`
    - `declarator_pointer_qualifier_staging_parse`
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `declarator_normal_tail_staging_parse`
    - `declarator_grouped_suffix_staging_parse`
    - `declarator_array_suffix_staging_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2188 total, 7 failed
    - `test_fail_after.log`: 2189 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 5 remains active after the parenthesized declarator
  inner-branch extraction; the next slice should decide whether the remaining
  parenthesized declarator suffix/finalization flow needs one more bounded
  helper before Step 5 can close
- This iteration's exact target: add parse-only coverage for a
  parenthesized pointer/member-pointer declarator that distinguishes the
  direct-name path from the nested-function-pointer recursion path, then
  extract that inner branch from `parse_parenthesized_pointer_declarator()`
  into one focused helper without changing suffix parsing or parameter
  capture behavior
- Next intended slice: inspect the remaining post-`RParen`
  parenthesized-pointer suffix/finalization flow and either extract one more
  focused helper or explicitly record Step 5 as sufficiently coordinatorized
- Completed: extracted the parenthesized pointer/member-pointer prefix setup
  from `parse_parenthesized_pointer_declarator()` into
  `parse_parenthesized_pointer_declarator_prefix()` so the outer function now
  reads as prefix/inner/suffix coordination without changing nested
  function-pointer or array-suffix behavior
- New helper path:
  - `parse_parenthesized_pointer_declarator_prefix(...)`
- Added parse-only coverage:
  - `declarator_parenthesized_member_pointer_prefix_parse`
- Baseline recorded:
  - `test_fail_before.log`: 2187 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused declarator regressions passed:
    - `declarator_parenthesized_member_pointer_prefix_parse`
    - `declarator_pointer_qualifier_staging_parse`
    - `declarator_parenthesized_fn_ptr_staging_parse`
    - `declarator_member_fn_ptr_suffix_staging_parse`
    - `declarator_normal_tail_staging_parse`
    - `declarator_grouped_suffix_staging_parse`
    - `declarator_array_suffix_staging_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2187 total, 7 failed
    - `test_fail_after.log`: 2188 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Active target: Step 5 remains active after the parenthesized declarator
  prefix extraction; the next slice should isolate the remaining
  inner-name-vs-nested-function-pointer branch inside
  `parse_parenthesized_pointer_declarator()` so `parse_declarator()` keeps
  converging on a staged coordinator
- This iteration's exact target: add parse-only coverage for an
  attribute-bearing parenthesized pointer-to-member declarator that keeps the
  current supported qualifier order stable, then extract the
  parenthesized pointer/member-pointer prefix setup from
  `parse_parenthesized_pointer_declarator()` into one focused helper without
  changing nested-function-pointer or array-suffix behavior
- Next intended slice: continue Step 5 by isolating the remaining
  parenthesized declarator inner-name vs nested-fn-ptr branch from
  `parse_parenthesized_pointer_declarator()`

- Completed: extracted the final record-member entry seam from
  `try_parse_record_member()` into `prepare_record_member_entry()` so the
  function now reads as a pure entry/prelude/template-dispatch coordinator
  without changing attribute skipping or end-of-body detection
- New helper path:
  - `prepare_record_member_entry(...)`
- Added parse-only coverage:
  - `record_member_entry_parse`
- Baseline recorded:
  - `test_before.log`: 2186 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused parser/member regressions passed:
    - `record_member_entry_parse`
    - `record_member_prelude_parse`
    - `record_member_mixed_prelude_parse`
    - `record_member_dispatch_parse`
    - `record_member_template_scope_cleanup_parse`
    - `record_member_specialization_context_parse`
    - `record_member_recovery_parse`
    - `record_body_loop_parse`
    - `record_body_context_parse`
    - `record_body_state_bundle_parse`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2186 total, 7 failed
    - `test_after.log`: 2187 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
  - clean rebuild note:
    - `cmake --build build -j8` hit an existing `build_example_ir` race after
      `rm -rf build`; rerunning as `cmake --build build -j1` completed the
      required clean rebuild without source changes
- Next intended slice: review the remaining active-plan frontier outside Step
  6 and choose the next smallest compression target still incomplete in
  `types.cpp`
- Active target: Step 6 is complete relative to the plan’s
  “record-body dispatcher” bar; Step 7 now tracks regression containment while
  the next compression cluster is selected
- This iteration's exact target: add parse-only coverage for attribute-bearing
  record members followed by a trailing access-label close, then extract the
  remaining `skip_attributes()` / end-of-body entry seam into one helper
- Completed: extracted the remaining access-label / friend / static_assert
  pre-dispatch chain from `try_parse_record_member()` into a focused helper so
  the function now reads as a thin prelude-or-template-dispatch coordinator
  without changing ordinary member resumption order
- New helper path:
  - `try_parse_record_member_prelude(...)`
- Added parse-only coverage:
  - `record_member_mixed_prelude_parse`
- Baseline recorded:
  - `test_before.log`: 2185 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused parser/member regressions passed:
    - `record_member_mixed_prelude_parse`
    - `record_member_prelude_parse`
    - `record_member_dispatch_parse`
    - `record_member_template_scope_cleanup_parse`
    - `record_body_context_parse`
    - `record_body_state_bundle_parse`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2185 total, 7 failed
    - `test_after.log`: 2186 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: inspect the remaining record-member entry path around
  `skip_attributes()` / end-of-body guarding and decide whether one more
  bounded Step 6 extraction is warranted or whether record-body parsing is now
  compressed enough to close Step 6 and advance the plan
- Active target: Step 6 remains active until the remaining record-member entry
  coordination is either extracted or explicitly judged complete relative to
  the plan’s “top-level dispatcher” bar
- This iteration's exact target: add parse-only coverage for a record body
  that interleaves access labels, friend/static_assert preludes, and ordinary
  members, then extract that remaining pre-dispatch prelude chain into one
  helper without changing which declarations are skipped or how the following
  member parses resume
- Completed: extracted the template-member prelude setup/teardown around
  `try_parse_record_member_dispatch(...)` from `try_parse_record_member()`
  into a focused helper without changing access-label/friend/static_assert
  handling, injected member-template type visibility, or cleanup before later
  ordinary/self-type record members
- New helper path:
  - `try_parse_record_member_with_template_prelude(...)`
- Added parse-only coverage:
  - `record_member_template_scope_cleanup_parse`
- Baseline recorded:
  - `test_before.log`: 2184 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused parser/member regressions passed:
    - `record_member_template_scope_cleanup_parse`
    - `record_member_prelude_parse`
    - `record_member_dispatch_parse`
    - `record_member_specialization_context_parse`
    - `record_body_context_parse`
    - `record_completion_handoff_parse`
    - `record_body_state_bundle_parse`
  - full clean rebuild `test_after.log` remained monotonic:
    - `test_before.log`: 2184 total, 7 failed
    - `test_after.log`: 2185 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting the remaining
  access-label / friend / static_assert prelude chain from
  `try_parse_record_member()` into one helper so the function keeps
  collapsing toward a pure coordinator before Step 6 closes
- Active target: Step 6 continues after the member-template prelude
  extraction; the next slice should pull the remaining access-label / friend /
  static_assert prelude chain out of `try_parse_record_member()` so the
  function reads as a thin prelude-or-dispatch coordinator
- This iteration's exact target: add parse-only coverage for a record body
  that interleaves access labels, friend/static_assert preludes, and ordinary
  members, then extract that remaining pre-dispatch prelude chain into one
  helper without changing which declarations are skipped or how the following
  member parses resume
- Completed: extracted the post-prelude record-member category chain from
  `try_parse_record_member()` into a focused
  `try_parse_record_member_dispatch(...)` helper without changing
  using/typedef/enum/nested-record ordering, special-member recognition, or
  template-member prelude scoping behavior
- New helper path:
  - `try_parse_record_member_dispatch(...)`
- Added parse-only coverage:
  - `record_member_specialization_context_parse`
- Validation completed:
  - focused parser/member regressions passed:
    - `record_member_specialization_context_parse`
    - `record_member_dispatch_parse`
    - `record_member_special_member_parse`
    - `record_body_context_parse`
    - `record_completion_handoff_parse`
    - `record_body_state_bundle_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2183 total, 7 failed
    - `test_fail_after.log`: 2184 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by compressing the remaining
  template-member prelude / scope-guard setup around
  `try_parse_record_member()` so the body loop keeps converging on a thin
  coordinator without changing injected template-parameter visibility
- Active target: Step 6 continues after the member-dispatch extraction; the
  next slice should isolate one more remaining `try_parse_record_member()`
  coordination concern, likely the local template-member prelude/guard
  lifecycle, without changing record-body behavior
- Completed: wrapped the record body field/method/member-typedef accumulators
  used by `parse_struct_or_union()` into a focused `RecordBodyState` helper
  bundle without changing member order, duplicate-field handling, or post-body
  finalization behavior
- New helper path:
  - `RecordBodyState`
- Added parse-only coverage:
  - `record_body_state_bundle_parse`
- Validation completed:
  - focused parser/body-state regressions passed:
    - `record_body_state_bundle_parse`
    - `record_completion_handoff_parse`
    - `record_body_context_parse`
    - `record_body_finalization_parse`
    - `record_definition_setup_parse`
    - `record_specialization_setup_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2181 total, 7 failed
    - `test_fail_after.log`: 2183 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+2` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting the remaining
  record-member prelude/category dispatch branching into one higher-level
  helper so both `parse_struct_or_union()` and the body loop remain
  coordinator-shaped
- Active target: Step 6 continues after record-body loop extraction; this
  slice extracts the remaining record-body context lifecycle from
  `parse_struct_or_union()` so the outer function reads more like a thin
  setup/body/finalization dispatcher
- This iteration's exact target: add parse-only coverage for a template
  specialization record body that contains a nested record and then resumes
  outer self-type parsing, then extract the existing body-context
  setup/parse/restore wrapper into a focused helper without changing nested
  record handling, constructor/destructor recognition, or self-type parsing
- Baseline recorded:
  - `test_fail_before.log`: 2180 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Completed: extracted the record body-context setup/parse/restore lifecycle
  from `parse_struct_or_union()` into a focused helper without changing nested
  record handling, template-specialization self-type injection, or
  constructor/destructor recognition after nested record bodies
- New helper path:
  - `parse_record_body_with_context(...)`
- Added parse-only coverage:
  - `record_body_context_parse`
- Validation completed:
  - focused parser/body-context regressions passed:
    - `record_body_context_parse`
    - `record_body_loop_parse`
    - `record_specialization_setup_parse`
    - `record_nested_aggregate_member_parse`
    - `record_body_finalization_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2180 total, 7 failed
    - `test_fail_after.log`: 2181 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting one more remaining
  `parse_struct_or_union()` coordinator concern, likely the local body-member
  accumulator setup / post-body storage handoff, so the outer function
  approaches a pure setup/body/finalization dispatcher
- Active target: Step 6 continues after record-body loop extraction; the next
  slice should pull the remaining body-context setup/teardown coordination out
  of `parse_struct_or_union()` so the outer function gets closer to a pure
  setup/body/finalization dispatcher
- Completed: extracted the record-body accumulator / dispatch / recovery loop
  from `parse_struct_or_union()` into a focused helper without changing member
  ordering, duplicate-field handling, or C++ recovery semantics
- New helper path:
  - `parse_record_body(...)`
- Added parse-only coverage:
  - `record_body_loop_parse`
- Baseline recorded:
  - `test_fail_before.log`: 2179 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused parser/body regressions passed:
    - `record_body_loop_parse`
    - `record_member_dispatch_parse`
    - `record_member_recovery_parse`
    - `record_member_method_field_parse`
    - `record_member_special_member_parse`
    - `record_body_finalization_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2179 total, 7 failed
    - `test_fail_after.log`: 2180 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting one more remaining
  `parse_struct_or_union()` coordinator concern, likely the body-context
  setup/restore around `current_struct_tag_` and template-origin self-type
  injection so the outer function keeps shrinking toward a thin dispatcher
- Active target: Step 6 continues after record-definition setup extraction;
  this slice extracts the remaining record-body loop wrapper from
  `parse_struct_or_union()` so the outer function reads as a thin
  setup/body/finalization coordinator
- This iteration's exact target: add parse-only coverage for a single record
  body that exercises both normal mixed member dispatch and malformed-member
  recovery, then extract the body-loop accumulator/recovery wrapper into a
  focused helper without changing member ordering, duplicate-field handling,
  or C++ recovery semantics
- Active target: Step 6 continues after record-definition setup extraction;
  the next slice should pull another remaining `parse_struct_or_union()`
  coordinator concern out of the function so it keeps collapsing toward a thin
  setup/body/finalization dispatcher
- Completed: extracted the record-definition node initialization from
  `parse_struct_or_union()` into a focused helper without changing template
  specialization storage, base-type transfer, or packed/aligned layout state
- New helper path:
  - `initialize_record_definition(...)`
- Added parse-only coverage:
  - `record_definition_setup_parse`
- Baseline recorded:
  - `test_fail_before.log`: 2178 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused parser/setup regressions passed:
    - `record_definition_setup_parse`
    - `record_prebody_setup_parse`
    - `record_tag_setup_parse`
    - `record_body_finalization_parse`
    - `record_specialization_setup_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2178 total, 7 failed
    - `test_fail_after.log`: 2179 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting one more remaining
  `parse_struct_or_union()` coordinator concern, likely the body-loop wrapper
  or the surrounding field/method accumulator setup so the outer function
  keeps shrinking toward a thin dispatcher
- Active target: Step 6 continues after the tag/setup extraction in
  `parse_struct_or_union()`; the next slice should pull one more remaining
  coordination concern out of the function so it keeps shrinking toward a thin
  setup/body/finalization dispatcher
- Completed: extracted the forward-reference versus body-definition tag/setup
  path from `parse_struct_or_union()` into a focused helper without changing
  anonymous tag synthesis, namespace qualification, forward-reference
  emission, or shadow-tag redefinition handling
- New helper path:
  - `parse_record_tag_setup(...)`
- Added parse-only coverage:
  - `record_tag_setup_parse`
- Baseline recorded:
  - `test_fail_before.log`: 2177 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - focused parser/tag regressions passed:
    - `record_tag_setup_parse`
    - `record_prebody_setup_parse`
    - `record_specialization_setup_parse`
    - `record_body_finalization_parse`
    - `namespace_struct_collision_runtime`
    - `namespace_struct_type_basic`
    - `namespace_template_struct_basic`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2177 total, 7 failed
    - `test_fail_after.log`: 2178 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting another remaining
  `parse_struct_or_union()` coordinator concern, likely the record-definition
  node initialization and specialization/base-type transfer just before the
  body loop
- Active target: Step 6 continues with pre-body setup compression in
  `parse_struct_or_union()`; this slice extracts one focused helper around the
  record prelude before `{` so the outer function keeps collapsing toward a
  setup/dispatch/finalization coordinator
- This iteration's exact target: add parse-only coverage for a record prelude
  that combines leading/trailing attributes, a template specialization tag, and
  base-clause parsing, then extract the existing pre-body setup path into a
  helper without changing attribute order, specialization detection, or base
  type collection
- Baseline recorded:
  - `test_fail_before.log`: 2176 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Completed: extracted the record pre-body setup path from
  `parse_struct_or_union()` into a focused helper without changing attribute
  capture order, specialization-tag handling, or base-clause parsing
- New helper path:
  - `parse_record_prebody_setup(...)`
- Added parse-only coverage:
  - `record_prebody_setup_parse`
- Validation completed:
  - focused parser/prelude regressions passed:
    - `record_prebody_setup_parse`
    - `record_specialization_setup_parse`
    - `record_body_finalization_parse`
    - `record_member_dispatch_parse`
    - `record_member_recovery_parse`
    - `template_struct_specialization_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2176 total, 7 failed
    - `test_fail_after.log`: 2177 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting one more
  `parse_struct_or_union()` pre-body concern, likely the forward-declaration /
  qualified-tag / shadow-tag decision path or the initial record-definition
  node setup before the body loop
- Completed: extracted the post-body finalization path from
  `parse_struct_or_union()` into focused helpers without changing trailing
  attribute probing, field/method/member-typedef storage, namespace
  qualification, or injected self-type registration
- New helper paths:
  - `apply_record_trailing_type_attributes(...)`
  - `store_record_body_members(...)`
  - `finalize_record_definition(...)`
- Added parse-only coverage:
  - `record_body_finalization_parse`
- Validation completed:
  - focused parser/member regressions passed:
    - `record_body_finalization_parse`
    - `record_specialization_setup_parse`
    - `record_member_dispatch_parse`
    - `record_member_special_member_parse`
    - `record_member_recovery_parse`
    - `record_member_method_field_parse`
    - `record_member_typedef_using_parse`
    - `record_member_enum_parse`
    - `record_member_prelude_parse`
    - `template_struct_specialization_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2175 total, 7 failed
    - `test_fail_after.log`: 2176 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting another remaining
  `parse_struct_or_union()` setup/teardown concern, likely attribute/tag/base
  setup before the body so the outer function keeps collapsing toward a thin
  coordinator around pre-body setup, member dispatch, and finalization
- Active target: Step 6 continues after record finalization extraction; the
  next slice should pull another pre-body coordination branch out of
  `parse_struct_or_union()` without changing parsing order or specialization
  behavior
- Completed: extracted the record-body injected self-type /
  template-specialization setup from `parse_struct_or_union()` into a focused
  helper without changing constructor/destructor recognition, scoped typedef
  registration, or specialization self-type parsing inside the record body
- New helper path:
  - `begin_record_body_context(...)`
- Added parse-only coverage:
  - `record_specialization_setup_parse`
- Validation completed:
  - focused parser/specialization regressions passed:
    - `record_specialization_setup_parse`
    - `record_member_dispatch_parse`
    - `record_member_special_member_parse`
    - `template_struct_specialization_parse`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2174 total, 7 failed
    - `test_fail_after.log`: 2175 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting another
  `parse_struct_or_union()` coordinator concern around body-finalization or
  pre-body setup so the outer function keeps shrinking toward setup,
  dispatcher, and teardown helpers
- Active target: Step 6 continues with record-body setup compression in
  `parse_struct_or_union()`; the next slice should extract the injected
  self-type/template-specialization setup so the outer function reads more like
  a coordinator around helperized setup, dispatch, and teardown
- This iteration's exact target: add parse-only coverage for a template
  specialization record body that relies on the specialization source name and
  injected self type inside constructors, destructors, typedefs, and method
  signatures, then extract that existing setup path into a focused helper
  without changing constructor/destructor recognition, scoped typedef
  registration, or self-type parsing inside the record body
- Baseline recorded:
  - `test_fail_before.log`: 2174 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Completed: extracted the record-body member-loop recovery path from
  `parse_struct_or_union()` into a focused helper without changing C++-only
  recovery semantics, brace-depth handling, or infinite-loop prevention
- New helper path:
  - `recover_record_member_parse_error(...)`
- Added parse-only coverage:
  - `record_member_recovery_parse`
- Validation completed:
  - focused parser/member regressions passed:
    - `record_member_recovery_parse`
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
    - `eastl_slice7c_struct_body_recovery`
  - full clean rebuild `test_fail_after.log` remained monotonic:
    - `test_fail_before.log`: 2174 total, 7 failed
    - `test_fail_after.log`: 2174 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+0` passed, `0` new failures, `0` new >30s
      tests)
- Next intended slice: continue Step 6 by extracting another
  `parse_struct_or_union()` coordinator concern outside the main body, likely a
  focused helper for attribute/tag/template-specialization setup before the
  record-body loop
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
