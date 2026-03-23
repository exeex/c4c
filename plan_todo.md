# Plan Todo — Template Function Identity

## Active Plan Item
Structured template function identity (plan.md Steps 1–6)

## Completed
- Step 1: Added FunctionTemplateEnv, SelectedFunctionTemplatePattern, FunctionTemplateInstanceKey types
- Step 2: Added owner-based specialization registry (function_specializations_ map in InstantiationRegistry)
- Step 3: Added select_function_specialization() structured selection helper
- Step 4: Switched lowering (initial + deferred) to use structured selection instead of find_specialization(mangled)
- Step 5: Added structured dedup (FunctionTemplateInstanceKey) for seed and instance recording
- Step 6: Old string-keyed path demoted to legacy fallback; structured selection is now the primary path

## Next Slice
All 6 steps complete. Plan acceptance criteria met:
1. Explicit fn-template specialization selection no longer primarily uses mangled_name
2. Owner-based specialization registration exists
3. Lowering selects from primary_def + bindings + nttp_bindings
4. Semantic dedup uses primary_def + spec_key (when primary_def available)
5. Mangled names remain only as derived output

## Baseline
2118/2123 tests passing (5 pre-existing failures, no regressions)
