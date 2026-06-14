Status: Active
Source Idea Path: ideas/open/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add or tighten route-native fail-closed diagnostic coverage

# Current Packet

## Just Finished

Step 2 - Add or tighten route-native fail-closed diagnostic coverage completed.

Changed implementation surface:

- `src/backend/mir/x86/debug/debug.cpp`: `append_route6_scalar_call_argument_sources(...)`
  now emits a route-native `route6 scalar arg status` row for named scalar
  `i32` call arguments when both prepared call-plan visibility and a Route 6
  source index are available. The row uses `route6_call_use_status_name(...)`
  for the raw Route 6 lookup status and a separate `gate=` token for the
  x86/prepared agreement gate.

Changed test surface:

- `tests/backend/bir/backend_x86_route_debug_test.cpp`: the selected scalar
  `i32` fixture now asserts named status rows for the positive `available`
  case and fail-closed absent Route 6, missing source id, duplicate Route 6,
  Route 6/prepared id mismatch, and source-name mismatch cases.

Observed route-debug rows for the selected matrix:

- Positive: `status=available gate=agreed`, followed by the existing
  `route6 scalar arg ... kind=ArgumentValue` row.
- Absent Route 6 relationship: `status=missing_source_relationship gate=blocked`.
- Missing Route 6 source id: `status=available gate=missing_source_value`.
- Duplicate Route 6 relationship: `status=duplicate_relationship gate=blocked`.
- Prepared source-id mismatch: `status=available gate=prepared_source_mismatch`.
- Source-name mismatch: `status=available gate=source_value_mismatch`.

Compatibility rows, prepared helper-oracle expectations, handoff boundary
expectations, and wrapper assembly were left unchanged.

## Suggested Next

Next coherent packet: Step 3 should introduce the smallest agreement-gated
Route 6 source-authority helper for the selected scalar `i32` direct-call
argument path, reusing the now-visible `gate=agreed` versus fail-closed
diagnostic states and preserving prepared fallback for every non-agreeing
case.

## Watchouts

- Do not broaden this into x86 call-wrapper migration or riscv convergence.
- Do not claim prepared aggregate or draft retirement.
- Do not weaken expected strings, helper-oracle statuses, supported-path
  contracts, fallback behavior, or wrapper assembly as proof.
- Route 6 source authority must be agreement-gated against the current
  prepared call argument source selection.
- The new status rows are diagnostic-only. They do not change wrapper
  authority, prepared source selection, fallback behavior, or `ConsumedPlans`
  compatibility.
- `status=available gate=missing_source_value` is intentionally a consumer-side
  agreement-gate result: the raw Route 6 lookup found a relationship, but the
  selected relationship lacks the source id required for x86/prepared
  agreement.
- `status=available gate=prepared_source_mismatch` and
  `status=available gate=source_value_mismatch` distinguish post-lookup
  agreement failures from raw Route 6 lookup failures.
- Keep `ConsumedPlans` public compatibility visible; do not hide or demote
  `PreparedFunctionLookups`, prepared call plans, or fallback labels in this
  idea.

## Proof

Passed:

`cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test backend_x86_handoff_boundary_test && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_prepared_lookup_helper|backend_x86_handoff_boundary)$' | tee test_after.log`

`test_after.log` is the proof log path. The selected subset passed 3/3:
`backend_x86_route_debug`, `backend_prepared_lookup_helper`, and
`backend_x86_handoff_boundary`.
