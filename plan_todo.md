# Plan Todo

- Status: Active
- Source Idea: [ideas/open/ast_to_hir_compression_plan.md](/workspaces/c4c/ideas/open/ast_to_hir_compression_plan.md)
- Source Plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 2: Compress Pending Template Struct Resolution

## Current Slice

- Completed: extracted the remaining field materialization path from
  `instantiate_template_struct_body(...)` so static-member bookkeeping and
  non-static field lowering now delegate to named helpers without changing
  substitution order or array-extent realization behavior
- Added focused regression coverage:
  - `cpp_hir_template_struct_field_array_extent` asserts
    `template_struct_nttp.cpp` still emits the concrete instantiated field
    layout line `field data: int[4] ... size=16 align=4`
- Completed: extracted shared helper(s) from
  `instantiate_template_struct_body(...)` for template-struct base type
  substitution / dependency seeding, typedef substitution, NTTP array-extent
  materialization, and per-method binding registration so the body instantiator
  now reads more like a coordinator without changing realization order
- Completed: extracted the shared specialization-binding and instance-key
  preparation helper from `resolve_pending_tpl_struct(...)` so the coordinator
  now delegates concrete instance identity staging before body instantiation
- Completed: extracted the shared post-realization helper path for seeding
  template-type dependency work and resolving deferred member typedefs once the
  owner tag is concrete
- Call sites now share helpers across:
  - `resolve_pending_tpl_struct(...)`
  - `instantiate_template_struct_body(...)`
  - `lower_struct_def(...)` base follow-up
- Added focused regression coverage:
  - `cpp_hir_template_member_owner_resolution` now covers direct
    `typename box<T>::value_type` lowering and reuse through an in-struct alias
  - tightened `cpp_hir_template_member_owner_resolution` to assert the concrete
    instantiated owner type appears in HIR as `decl w: struct wrap_T_int`
- Baseline recorded for this iteration:
  - `test_fail_before.log`: 2212 total, 7 failed
  - failing identities:
    - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
    - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
    - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
    - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
    - `cpp_positive_sema_template_arg_deduction_cpp`
    - `cpp_positive_sema_template_mixed_params_cpp`
    - `cpp_positive_sema_template_type_subst_cpp`
- Validation completed:
  - targeted HIR regressions passed:
    - `cpp_hir_template_struct_field_array_extent`
    - `cpp_hir_template_member_owner_resolution`
    - `cpp_hir_template_struct_inherited_method_binding`
    - `cpp_hir_deferred_template_instantiation`
    - `cpp_hir_template_struct_registry_primary_only`
    - `cpp_hir_initial_program_seed_realization`
    - `cpp_hir_fixpoint_convergence`
    - `cpp_hir_template_origin`
  - full clean rebuild remained monotonic:
    - `test_fail_before.log`: 2212 total, 7 failed
    - `test_fail_after.log`: 2214 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+2` passed, `0` new failures)
  - added focused HIR-only regression coverage:
    - `tests/cpp/internal/hir_case/template_struct_inherited_method_hir.cpp`
      checks a concrete `Base<int>` / `Wrap<int>` instantiation with inherited
      member access inside an instantiated method body without enrolling a new
      positive runtime case

## Next Intended Slice

- if this extraction stays monotonic, continue Step 2 by checking whether the
  remaining `instantiate_deferred_template_type(...)` / pending-template entry
  path can share the same coordinator helpers without changing retry behavior;
  if that is already compact enough, move to the next repeated pending-template
  sequence inside `resolve_pending_tpl_struct_if_needed(...)`

## Blockers

- none

## Resume Notes

- Step 1 coordinator extraction is already complete enough to advance
- the previous Step 2 slice in `instantiate_deferred_template_type(...)` is
  already committed as `2d6b0e3`
- this iteration is deliberately limited to helper extraction around
  the remaining field/static-member materialization path inside
  `instantiate_template_struct_body(...)`, not semantic cleanup
- next Step 2 candidate is the remaining pending-template entry coordination
  around `instantiate_deferred_template_type(...)` and
  `resolve_pending_tpl_struct_if_needed(...)`; keep any follow-up extraction
  behavior-preserving and reuse the current targeted HIR regression set
