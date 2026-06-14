Status: Active
Source Idea Path: ideas/open/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish the x86 Route 6 diagnostic/oracle baseline

# Current Packet

## Just Finished

Step 1 - Establish the x86 Route 6 diagnostic/oracle baseline completed as an
analysis-only packet.

Inspected implementation surfaces:

- `src/backend/mir/x86/x86.hpp`: `ConsumedPlans`,
  `consume_plans(...)`, `find_consumed_call_argument_plan(...)`, and
  `find_consumed_scalar_i32_call_argument_source(...)`.
- `src/backend/mir/x86/debug/debug.cpp`:
  `append_route6_scalar_call_argument_sources(...)` and `render_report(...)`.
- `src/backend/mir/x86/module/module.cpp`:
  `append_prepared_direct_extern_call_argument(...)`,
  `append_prepared_direct_extern_call_return_function(...)`, and the direct
  extern call argument paths that pass optional Route 6 source records into
  the prepared fallback emitter.
- `src/backend/bir/bir.cpp` / `src/backend/bir/bir.hpp`:
  `Route6CallUseStatus`, `route6_call_argument_source_record(...)`,
  `route6_find_call_argument_source(...)`,
  `route6_call_argument_source_matches_argument_value_record(...)`,
  `route6_call_argument_source_producer_record(...)`, and indexed Route 6
  lookup builders.

Inspected test surfaces:

- `tests/backend/bir/backend_x86_route_debug_test.cpp`: fixture
  `legalize_single_block_same_module_scalar_call_wrapper_miss_module()`,
  helpers `selected_route6_scalar_arg_without_route6_facts()`,
  `selected_route6_scalar_arg_with_invalid_source_id()`,
  `selected_route6_scalar_arg_with_duplicate_conflict()`,
  `selected_route6_scalar_arg_with_prepared_mismatch()`, and
  `selected_route6_scalar_arg_with_source_name_mismatch()`, plus the expected
  positive route-debug row
  `route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue`.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`: x86
  `ConsumedPlans` helper coverage around `x86_route6_consumed_plans` and
  generic Route 6 helper-oracle coverage in
  `verify_bir_call_argument_source_producer_materialization_lookup()`.
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`:
  `expected_minimal_direct_extern_call_lane_asm()`,
  `check_consumed_plans_threads_route6_scalar_call_argument_source()`,
  prepared fallback asm stability, and authoritative prepared
  BeforeCall/AfterCall bundle checks.
- `tests/backend/bir/CMakeLists.txt`: test names
  `backend_x86_route_debug`, `backend_x86_handoff_boundary`, and
  `backend_prepared_lookup_helper`.

Baseline findings:

- Positive x86 Route 6 route-debug visibility exists only for the selected
  scalar `i32` same-module call fixture: the trace emits one row when
  `find_consumed_scalar_i32_call_argument_source(...)` returns an agreed
  `ArgumentValue` record.
- Current x86 route-debug fail-closed coverage for absent Route 6, invalid
  source id, duplicate Route 6, Route 6/prepared id mismatch, and source-name
  mismatch is absence-based: each fixture still prints the function row but
  must not print the positive Route 6 scalar arg row.
- Helper-oracle/status coverage exists in `backend_prepared_lookup_helper` for
  `available`, `missing_argument`, `wrong_call`,
  `missing_source_relationship`, `abi_bound_excluded`,
  `duplicate_relationship`, and `no_match` on Route 6 call argument source or
  producer lookups. `missing_source_value` is represented in generic Route 6
  result-source coverage, but the inspected call-argument producer path reports
  the unnamed/immediate argument case as `missing_source_producer`, not
  `missing_source_value`.
- x86 `ConsumedPlans` compatibility currently proves agreed scalar `i32`
  `ArgumentValue`, source-name mismatch fail-closed, absent Route 6
  fail-closed with prepared call argument selection preserved, and
  Route 6/prepared source-id mismatch fail-closed with prepared selection
  preserved.
- `expected_minimal_direct_extern_call_lane_asm()` is the wrapper-output
  stability oracle. Prepared fallback currently remains byte-stable when Route
  6 facts are removed from the `printf` scalar argument source path.
- The implementation already passes optional Route 6 source records into
  `append_prepared_direct_extern_call_argument(...)`, but final emission still
  depends on prepared homes and prepared BeforeCall/AfterCall bundles.

## Suggested Next

First code-changing packet: Step 2 should add route-native named
diagnostic/status observability for the selected x86 scalar `i32` Route 6
route-debug fail-closed matrix before changing wrapper authority.

Recommended owned surfaces for that packet:

- Implementation: `src/backend/mir/x86/debug/debug.cpp`, extending
  `append_route6_scalar_call_argument_sources(...)` or a narrow adjacent helper
  so it can print selected Route 6 status rows for the existing fixture without
  changing wrapper assembly or fallback behavior.
- Tests: `tests/backend/bir/backend_x86_route_debug_test.cpp`, adding expected
  named rows for current missing/invalid/duplicate/mismatch/source-name cases
  beside the existing positive row and absence assertions.
- Compatibility guard surfaces to leave byte-stable:
  `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
  `expected_minimal_direct_extern_call_lane_asm()`,
  `check_consumed_plans_threads_route6_scalar_call_argument_source()`, and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` x86
  `ConsumedPlans` plus generic Route 6 helper-oracle coverage.

Suggested narrow proof for the code-changing packet:
`cmake --build build --target backend_x86_route_debug_test backend_prepared_lookup_helper_test backend_x86_handoff_boundary_test && ctest --test-dir build -R '^(backend_x86_route_debug|backend_prepared_lookup_helper|backend_x86_handoff_boundary)$' --output-on-failure`.

## Watchouts

- Do not broaden this into x86 call-wrapper migration or riscv convergence.
- Do not claim prepared aggregate or draft retirement.
- Do not weaken expected strings, helper-oracle statuses, supported-path
  contracts, fallback behavior, or wrapper assembly as proof.
- Route 6 source authority must be agreement-gated against the current
  prepared call argument source selection.
- Missing fail-closed route-debug coverage before code changes: there are no
  named x86 route-debug status rows yet for absent Route 6, invalid/missing
  source id, duplicate Route 6, Route 6/prepared id mismatch, source-name
  mismatch, wrong call, wrong argument, missing source relationship, or missing
  source value. The current x86 route-debug proof is positive-row present vs.
  absent-row fail-closed only.
- Keep `ConsumedPlans` public compatibility visible; do not hide or demote
  `PreparedFunctionLookups`, prepared call plans, or fallback labels in this
  idea.
- Treat `missing_source_value` carefully: the route status enum has the value,
  but the inspected call-argument producer path reports unnamed/immediate
  source arguments as `missing_source_producer`; adding a call-argument
  `missing_source_value` diagnostic may require a focused test expectation for
  the intended semantic distinction.

## Proof

Analysis-only packet. No build or ctest proof required and no `test_after.log`
generated.

Read-only validation used: source/test inspection, `rg`, `git diff -- todo.md`,
`git status --short`, and AST-backed `c4c-clang-tool` /
`c4c-clang-tool-ccdb` signature/caller queries for x86 `ConsumedPlans`,
`find_consumed_scalar_i32_call_argument_source(...)`, and
`append_route6_scalar_call_argument_sources(...)`.
