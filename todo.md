Status: Active
Source Idea Path: ideas/open/214_route6_x86_scalar_source_route_debug_row.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Pin the Selected Debug Row

# Current Packet

## Just Finished

Step 1: Pin the Selected Debug Row completed the analysis-only row selection for active idea 214.

Selected route-debug row:

```text
    route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue
```

Selected fixture:

- `tests/backend/bir/backend_x86_route_debug_test.cpp`: `legalize_single_block_same_module_scalar_call_wrapper_miss_module()`.
- Function/block/call key: `single_block_same_module_scalar_call_wrapper_miss`, block `entry`, instruction index `2`, callee `addip0`, argument index `0`, argument `%t1` typed `i32`, result `%t2`.
- Summary/trace surface already materializes `single_block_same_module_scalar_call_wrapper_miss_summary` and `single_block_same_module_scalar_call_wrapper_miss_trace`; current expectations do not assert a scalar Route 6 row for that fixture.

Expected text owner and route mapping:

- `src/backend/mir/x86/debug/debug.cpp` owns route-debug text in `append_grouped_authority(...)` and the summary/trace function loops. Current behavior consumes prepared plans via `x86::consume_plans(...)` and emits grouped authority rows only; it does not call the scalar Route 6 helper for the selected call.
- `src/backend/mir/x86/x86.hpp` owns the existing agreement helper: `find_consumed_scalar_i32_call_argument_source(...)`. It accepts only a prepared call argument plus present Route 6 index plus named `i32` argument plus `Route6CallUseSourceKind::ArgumentValue` with matching prepared/source ids.
- The selected row should be emitted by `debug.cpp` only when that helper accepts the current `ConsumedPlans`/Route 6 agreement. Wrapper kind, ABI placement, moves, and call-plan ownership stay prepared-backed.

Current prepared-backed behavior:

- Positive Route 6 scalar agreement exists only in helper coverage today: `backend_prepared_lookup_helper_test.cpp` constructs `x86_route6_consumed_plans` with callee `consume_x86_scalar`, arg `%x86.scalar`, `source_value_id=301`, and proves the helper returns `ArgumentValue`.
- The x86 route-debug renderer currently has no byte-stable assertion for the selected scalar source row; the selected fixture strings are computed but not checked for scalar Route 6 text.

Coverage and gaps recorded for Step 2/3:

- Positive: existing helper proof covers agreed scalar `i32` `ArgumentValue` with matching prepared/source ids.
- Absent: helper proof resets `route6_call_use_sources` and proves the Route 6 helper fails closed while `find_consumed_call_argument_plan(...)` still returns the prepared argument.
- Invalid: helper code rejects missing prepared argument, missing/empty Route 6 index, non-named argument, and non-`i32` argument; there is no dedicated x86 route-debug expected-string proof yet.
- Duplicate/conflict: generic Route 6 call-argument source record/index coverage proves duplicate relationships report `DuplicateRelationship`; x86-specific duplicate fallback still needs route-debug proof.
- Mismatch: helper proof changes the prepared source id from `301` to `302`, proving Route 6/prepared disagreement fails closed and prepared argument lookup remains available.
- Fallback: current compatibility boundary is `ConsumedPlans`; prepared call argument selection remains available for absent and mismatch cases.
- Wrapper no-change proof candidates for later implementation: `backend_x86_handoff_boundary_direct_extern_call`, `backend_x86_handoff_boundary_lir`, plus `backend_x86_route_debug` focused on this fixture once x86 backend tests are enabled. No wrapper expectation rewrite should be part of this idea.
- Missing proof gaps: byte-stable x86 route-debug positive row; x86 route-debug absent/invalid/duplicate/mismatch/fallback text; exact numeric prepared source id for `%t1` in the selected fixture if Step 2 chooses to print ids. Prefer the pinned row text above to avoid making the public debug contract depend on an unobserved numeric id.

## Suggested Next

Proceed to Step 2: add the route-native row selection in `src/backend/mir/x86/debug/debug.cpp` for only the selected `single_block_same_module_scalar_call_wrapper_miss` scalar `i32` call-argument agreement path, using `find_consumed_scalar_i32_call_argument_source(...)` as the acceptance gate and preserving prepared fallback behavior.

## Watchouts

- Keep this to one route-debug row.
- Do not migrate x86 wrappers, call plans, call printer output, ABI policy, or `ConsumedPlans`.
- Treat expected-string rewrites, helper renames, unsupported downgrades, and narrow testcase-shaped matching as blockers.
- Current `build/CMakeCache.txt` has `C4C_ENABLE_X86_BACKEND_TESTS:BOOL=OFF`; Step 2/3 either need an x86-enabled configured build or an alternate supervisor-approved configured proof. Current CTest discovery lists `backend_prepared_lookup_helper` but not `backend_x86_route_debug`.
- Do not broaden from the selected `i32` argument row to `i64`, aggregate, result, lane, direct-global, or publication-source Route 6 families.
- If implementation prints a numeric source id, first prove the exact `%t1` prepared value id under an x86-enabled route-debug fixture instead of guessing it.

## Proof

Analysis-only selection packet; no build required and no validation log was created because validation logs were out of scope for Step 1.

Recommended minimal proof command for later implementation:

```sh
cmake --build build --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure
```

If the active build remains configured with `C4C_ENABLE_X86_BACKEND_TESTS:BOOL=OFF`, `backend_x86_route_debug_test` is unavailable; reconfigure an x86-enabled build or use a separate x86-enabled build directory before treating Step 2/3 as proven.
