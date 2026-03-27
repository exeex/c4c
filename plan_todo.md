# Plan Todo

- Status: Active
- Source Idea: [ideas/open/ast_to_hir_compression_plan.md](/workspaces/c4c/ideas/open/ast_to_hir_compression_plan.md)
- Source Plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 2: Compress Pending Template Struct Resolution

## Current Slice

- Completed: extracted the repeated owner-primary lookup, blocked/terminal
  diagnostic formatting, and owner-struct retry flow out of
  `instantiate_deferred_template_type(...)` without changing pending-template
  semantics
- New helper paths:
  - contextual deferred-template diagnostic formatting
  - primary-template lookup for owner/member-typedef work items
  - shared owner-struct resolution/retry gate
- Added focused regression coverage:
  - `cpp_hir_template_member_owner_resolution`
- Baseline recorded:
  - `test_before.log`: 2210 total, 7 failed
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
    - `cpp_hir_template_member_owner_resolution`
    - `cpp_hir_deferred_template_instantiation`
    - `cpp_hir_template_struct_registry_primary_only`
    - `cpp_hir_initial_program_seed_realization`
    - `cpp_hir_fixpoint_convergence`
    - `cpp_hir_template_origin`
  - full clean rebuild remained monotonic:
    - `test_before.log`: 2210 total, 7 failed
    - `test_after.log`: 2212 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+2` passed, `0` new failures)

## Next Intended Slice

- after this helper extraction lands, move to the next repeated pending-template
  path shared by `resolve_pending_tpl_struct(...)` and
  `instantiate_template_struct_body(...)`

## Blockers

- none

## Resume Notes

- Step 1 coordinator extraction is already complete enough to advance; this
  iteration completed the first Step 2 helper slice inside
  `instantiate_deferred_template_type(...)`
- untracked local files in the repo root are unrelated scratch binaries and
  source files; leave them untouched
