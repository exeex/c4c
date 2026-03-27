# Plan Todo

- Status: Active
- Source Idea: [ideas/open/ast_to_hir_compression_plan.md](/workspaces/c4c/ideas/open/ast_to_hir_compression_plan.md)
- Source Plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 4: Isolate Embedded Parser Helpers

## Current Slice

- Completed: isolated the deferred NTTP default-expression text parser used by
  `eval_deferred_nttp_expr_hir(...)` behind the named helper
  `DeferredNttpExprParser`, including a dedicated helper path for template
  static-member lookup staging inside the parser surface
- Completed: tightened the static-member evaluator used by the embedded parser
  so it now scans either `fields[]` or `children[]`, matching the mixed record
  storage shapes already handled elsewhere in `ast_to_hir.cpp`
- Added focused HIR coverage for this slice:
  - `cpp_hir_template_deferred_nttp_expr` asserts a deferred NTTP default that
    reuses a previously bound NTTP (`M = N`) still materializes as
    `field data: int[3] ... size=12 align=4`
- Discovered separate follow-on initiative during this slice:
  - deferred NTTP text expressions that require arithmetic (`N + 2`) or
    template static-member lookup (`Count<N>::value + 2`) still collapse to
    `..._M_0` in HIR lowering
  - recorded separately in
    `ideas/open/deferred_nttp_expr_parser_gaps.md`
- Step 3 closed: the remaining differences between `lower_function(...)` and
  `lower_struct_method(...)` are path-specific body work rather than
  mechanically shared setup worth further extraction in this refactor
- Completed: extracted the remaining shared callable
  registration/finalization seam from `lower_function(...)` and
  `lower_struct_method(...)` via:
  - `maybe_register_bodyless_callable(...)` for the declaration-only fast path
  - `finish_lowered_callable(...)` for empty-block finalization and final
    function installation
- Shared setup now covers:
  - declaration-only callable registration in both free-function and method
    lowering
  - final lowered-function installation for normal functions, defaulted
    methods, and regular methods
- Deliberately left in place for later Step 3 reassessment:
  - function-only VLA/array-size side-effect lowering
  - method-only defaulted-body emission
  - constructor initializer-list lowering
  - destructor epilogue emission
- Added focused HIR coverage for this slice:
  - `cpp_hir_defaulted_destructor_member_teardown` asserts the defaulted
    `Outer` destructor still emits the member teardown call
    `WithDtor__dtor((&this#P0->inner))`
- Completed: extracted the shared callable body-entry helpers
  `register_bodyless_callable(...)` and `begin_callable_body_lowering(...)`
  so both `lower_function(...)` and `lower_struct_method(...)` now share:
  - declaration-only callable registration into `module_->fn_index`
  - temporary skeleton installation before body lowering when needed
  - entry-block creation/assignment before path-specific body work
- Deliberately left in place for later Step 3 slices:
  - function-only VLA/array-size side-effect lowering
  - method-only defaulted-body emission
  - constructor initializer-list lowering
  - destructor epilogue emission
- Added focused HIR coverage for this slice:
  - `cpp_hir_template_function_recursive_body_binding` asserts a templated
    recursive function body still lowers the recursive instantiated call as
    `return countdown<int>((n#P0 - 1))`
- Completed: extracted shared explicit-parameter lowering coordinator
  `append_callable_params(...)` and routed both `lower_function(...)` and
  `lower_struct_method(...)` through it while keeping:
  - function-only parameter-pack expansion in `lower_function(...)`
  - method-only implicit `this` injection and body prelude work in
    `lower_struct_method(...)`
  - the existing distinction between function signature typing and
    method-only typedef-to-struct parameter resolution
- Added focused HIR coverage for this slice:
  - `cpp_hir_template_function_pack_signature_binding` asserts a variadic
    template function instantiation lowers its expanded parameters as
    `args__pack0: int, args__pack1: long`
- Completed assessment: `resolve_pending_tpl_struct_if_needed(...)` is already a
  thin wrapper over the Step 2 helpers, so there was no remaining substantial
  extraction slice there without dropping into cosmetic churn
- Completed: extracted shared Step 3 helper paths for callable signature setup:
  - `substitute_signature_template_type(...)`
  - `resolve_signature_template_type_if_needed(...)`
  - `prepare_callable_return_type(...)`
  - `append_explicit_callable_param(...)`
- Shared setup now covers:
  - free-function return-type preparation in `lower_function(...)`
  - struct-method return-type preparation in `lower_struct_method(...)`
  - explicit parameter emission in both paths
- Deliberately left in place for later Step 3 slices:
  - parameter-pack expansion ownership in `lower_function(...)`
  - implicit `this` injection and method-only body prelude work in
    `lower_struct_method(...)`
- Added focused HIR coverage for this slice:
  - `cpp_hir_template_function_signature_binding` asserts
    `make_pair<int>` lowers as
    `fn make_pair_i(a: int, b: int) -> struct Pair_T_int`
  - `cpp_hir_template_method_signature_binding` asserts
    `Transformer<int>::make_scaled_pair(int)` lowers as
    `fn Transformer_T_int__make_scaled_pair(this: struct Transformer_T_int*, x: int) -> struct Pair_T_int`
- Completed: extracted shared helper
  `seed_and_resolve_pending_template_type_if_needed(...)` so repeated
  template-owner / member-typedef dependent declaration paths now share one
  seed-and-resolve sequence without changing pending-owner retry behavior
- Call sites converted in this slice:
  - generic-control-type inference
  - nested template arg materialization
  - struct base realization during lowering
  - member typedef binding application
  - function and method return / parameter declaration types
  - local declarations
  - named template-owner materialization for constructor-style and scoped
    references
  - cast targets
- Added focused regression coverage:
  - `cpp_hir_template_member_owner_decl_and_cast` asserts a templated
    member-typedef chain lowers concretely in both a local declaration
    (`decl local: int = input#P0`) and a cast target (`return ((int)local#L0)`)
- Completed: extracted the remaining deferred-template entry coordination from
  `instantiate_deferred_template_type(...)` so direct owner-struct retries,
  delegated owner work spawning, and member-typedef owner resolution all flow
  through shared helpers without changing retry/blocked behavior
- Added focused regression coverage for this slice:
  - `cpp_hir_template_member_owner_chain` will assert a nested
    `typename holder<T>::inner` field lowers concretely as
    `field value: int llvm_idx=0 offset=0 size=4 align=4`
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
  - `test_fail_before.log`: 2215 total, 7 failed
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
  - targeted HIR regressions passed:
    - `cpp_hir_template_member_owner_chain`
    - `cpp_hir_template_member_owner_resolution`
    - `cpp_hir_template_struct_inherited_method_binding`
    - `cpp_hir_deferred_template_instantiation`
    - `cpp_hir_template_struct_registry_primary_only`
    - `cpp_hir_initial_program_seed_realization`
    - `cpp_hir_fixpoint_convergence`
    - `cpp_hir_template_origin`
    - `cpp_hir_template_struct_field_array_extent`
  - full clean rebuild remained monotonic:
    - `test_fail_before.log`: 2214 total, 7 failed
    - `test_fail_after.log`: 2215 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)
  - targeted HIR regressions passed:
    - `cpp_hir_template_member_owner_decl_and_cast`
    - `cpp_hir_template_member_owner_chain`
    - `cpp_hir_template_member_owner_resolution`
    - `cpp_hir_template_struct_inherited_method_binding`
    - `cpp_hir_template_struct_field_array_extent`
    - `cpp_hir_deferred_template_instantiation`
    - `cpp_hir_template_struct_registry_primary_only`
    - `cpp_hir_initial_program_seed_realization`
    - `cpp_hir_fixpoint_convergence`
    - `cpp_hir_template_origin`
  - full clean rebuild remained monotonic:
    - `test_fail_before.log`: 2215 total, 7 failed
    - `test_fail_after.log`: 2216 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)
  - targeted HIR regressions passed:
    - `cpp_hir_template_function_signature_binding`
    - `cpp_hir_template_method_signature_binding`
    - `cpp_hir_template_member_owner_decl_and_cast`
    - `cpp_hir_template_member_owner_chain`
    - `cpp_hir_template_member_owner_resolution`
    - `cpp_hir_template_struct_inherited_method_binding`
    - `cpp_hir_template_struct_field_array_extent`
    - `cpp_hir_deferred_template_instantiation`
    - `cpp_hir_template_struct_registry_primary_only`
    - `cpp_hir_initial_program_seed_realization`
    - `cpp_hir_fixpoint_convergence`
    - `cpp_hir_template_origin`
  - full clean rebuild remained monotonic:
    - `test_fail_before.log`: 2216 total, 7 failed
    - `test_fail_after.log`: 2218 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+2` passed, `0` new failures)

## Next Intended Slice

- extract the next embedded parser seam after the deferred NTTP expression
  parser if this slice lands cleanly
- keep Step 4 focused on helper isolation rather than expression-semantics
  changes

## Blockers

- none

## Resume Notes

- Step 1 coordinator extraction is already complete enough to advance
- the previous Step 2 slice in `instantiate_deferred_template_type(...)` is
  already committed as `2d6b0e3`
- this iteration is deliberately limited to helper extraction around
  deferred template-type owner handling inside
  `instantiate_deferred_template_type(...)`, not semantic cleanup
- the new focused HIR coverage should exercise a nested owner/member-typedef
  chain rather than only the direct `box<T>::value_type` case
- Step 2 is complete enough to leave behind
- Step 3 is now complete enough to leave behind; the next meaningful work is
  Step 4 embedded parser helper isolation around deferred NTTP expression
  evaluation
- Validation completed for this slice:
  - targeted HIR regressions passed:
    - `cpp_hir_template_deferred_nttp_expr`
    - `cpp_hir_template_member_owner_resolution`
    - `cpp_hir_template_function_recursive_body_binding`
    - `cpp_hir_defaulted_destructor_member_teardown`
  - full clean rebuild remained monotonic:
    - `test_fail_before.log`: 2221 total, 7 failed
    - `test_fail_after.log`: 2222 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)
  helper reshuffling
- this slice completed the shared callable return/explicit-parameter helper
  extraction without changing pack expansion or method-only prologue behavior
- current follow-up is narrower than a semantic cleanup: factor the remaining
  explicit-parameter loop/control flow into one shared helper and prove that
  method parameter typedef resolution still stays method-only
- this slice is now complete: explicit parameter iteration/pack expansion is
  centralized in `append_callable_params(...)`, and the new pack-signature HIR
  test is the direct regression guard for that coordinator
- current slice starts from the remaining shared callable setup around
  pre-registration/skeleton installation/entry-block creation; any extraction
  must preserve the existing function self-reference ordering and the method
  defaulted/ctor/dtor split points
- validation completed:
  - targeted regressions passed:
    - `cpp_hir_template_function_recursive_body_binding`
    - `cpp_hir_template_function_pack_signature_binding`
    - `cpp_hir_template_function_signature_binding`
    - `cpp_hir_template_method_signature_binding`
    - `cpp_positive_sema_default_special_members_cpp`
  - full clean rebuild remained monotonic:
    - `test_before.log`: 2219 total, 7 failed
    - `test_after.log`: 2220 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)
- validation completed:
  - targeted regressions passed:
    - `cpp_hir_defaulted_destructor_member_teardown`
    - `cpp_hir_template_function_recursive_body_binding`
    - `cpp_hir_template_function_pack_signature_binding`
    - `cpp_hir_template_function_signature_binding`
    - `cpp_hir_template_method_signature_binding`
    - `cpp_positive_sema_default_special_members_cpp`
  - full clean rebuild remained monotonic:
    - `test_before.log`: 2220 total, 7 failed
    - `test_after.log`: 2221 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)
- validation completed:
  - targeted HIR regressions passed:
    - `cpp_hir_template_function_pack_signature_binding`
    - `cpp_hir_template_function_signature_binding`
    - `cpp_hir_template_method_signature_binding`
    - `cpp_hir_template_member_owner_decl_and_cast`
    - `cpp_hir_template_member_owner_chain`
    - `cpp_hir_template_member_owner_resolution`
    - `cpp_hir_template_struct_inherited_method_binding`
    - `cpp_hir_template_struct_field_array_extent`
    - `cpp_hir_deferred_template_instantiation`
    - `cpp_hir_template_struct_registry_primary_only`
    - `cpp_hir_initial_program_seed_realization`
    - `cpp_hir_fixpoint_convergence`
    - `cpp_hir_template_origin`
  - full clean rebuild remained monotonic:
    - `test_before.log`: 2218 total, 7 failed
    - `test_after.log`: 2219 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)
