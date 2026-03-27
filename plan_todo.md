# Plan Todo

- Status: Active
- Source Idea: [ideas/open/ast_to_hir_compression_plan.md](/workspaces/c4c/ideas/open/ast_to_hir_compression_plan.md)
- Source Plan: [plan.md](/workspaces/c4c/plan.md)

## Active Item

- Step 1: Compress Initial Program Orchestration

## Current Slice

- Completed: extracted the top-level item flattening, weak-symbol collection,
  type-definition collection, consteval/template registration, template seed
  fixpoint, metadata population, ref-overload discovery, out-of-class method
  attachment, and non-method lowering passes from
  `Lowerer::lower_initial_program(...)` into named helpers so the entrypoint now
  reads as a phase coordinator without changing phase order or lowering
  behavior
- New helper paths:
  - `flatten_program_items(...)`
  - `collect_weak_symbol_names(...)`
  - `collect_initial_type_definitions(...)`
  - `collect_consteval_function_definitions(...)`
  - `collect_template_function_definitions(...)`
  - `collect_function_template_specializations(...)`
  - `collect_depth0_template_instantiations(...)`
  - `run_consteval_template_seed_fixpoint(...)`
  - `finalize_template_seed_realization(...)`
  - `populate_hir_template_defs(...)`
  - `collect_ref_overloaded_free_functions(...)`
  - `attach_out_of_class_struct_method_defs(...)`
  - `lower_non_method_functions_and_globals(...)`
  - `lower_pending_struct_methods(...)`
- Added focused HIR coverage:
  - `cpp_hir_initial_program_seed_realization`
- Baseline recorded:
  - `test_before.log`: 2209 total, 7 failed
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
    - `cpp_hir_initial_program_seed_realization`
    - `cpp_hir_materialization_boundary`
    - `cpp_hir_template_origin`
    - `cpp_hir_fixpoint_convergence`
    - `cpp_hir_deferred_template_instantiation`
    - `cpp_hir_template_struct_registry_primary_only`
  - full clean rebuild remained monotonic:
    - `test_before.log`: 2209 total, 7 failed
    - `test_after.log`: 2210 total, 7 failed
    - failing identities unchanged
    - regression guard: passed (`+1` passed, `0` new failures)

## Next Intended Slice

- inspect whether Step 1 should extract the remaining Phase 2/2.5 lowering
  comments into slightly tighter coordinator helpers or whether the current
  `lower_initial_program(...)` shape is sufficient to advance to Step 2

## Blockers

- none

## Resume Notes

- no active `plan.md`/`plan_todo.md` existed at turn start, so this runbook was
  activated from `ideas/open/ast_to_hir_compression_plan.md`
- untracked local files in the repo root are unrelated scratch binaries and
  source files; leave them untouched
