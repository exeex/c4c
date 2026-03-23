# Plan Todo — Template Function Identity

## Status: COMPLETE (2026-03-24)

## Plan Item
Structured template function identity (plan.md Steps 1–6)

## Completed
- Step 1: Added FunctionTemplateEnv, SelectedFunctionTemplatePattern, FunctionTemplateInstanceKey types
- Step 2: Added owner-based specialization registry (function_specializations_ map in InstantiationRegistry)
- Step 3: Added select_function_specialization() structured selection helper
- Step 4: Switched lowering (initial + deferred) to use structured selection instead of find_specialization(mangled)
- Step 5: Added structured dedup (FunctionTemplateInstanceKey) for seed and instance recording
- Step 6: Old string-keyed path demoted to legacy fallback; structured selection is now the primary path

## Post-Completion Fixes (2026-03-24)
- Fixed cpp_hir_fixpoint_convergence test regex (new "template types resolved" stats field)
- Fixed typedef-wrapped struct rvalue member access in codegen (operator_shift_call_arg_runtime)

## Final Baseline
2120/2123 tests passing (3 pre-existing failures, no regressions)

## Next
Plan fully delivered. Next plan TBD (template_lazy_instantiation_plan.md is the natural successor).
