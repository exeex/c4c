Status: Active
Source Idea Path: ideas/open/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Introduce agreement-gated Route 6 source authority

# Current Packet

## Just Finished

Step 3 - Introduce agreement-gated Route 6 source authority completed.

Changed implementation surface:

- `src/backend/mir/x86/x86.hpp`: added
  `find_consumed_scalar_i32_call_argument_source_authority(...)`, which only
  returns Route 6 authority for the selected scalar `i32` call-argument path
  when the Route 6 argument-value record matches the BIR argument, has a
  source id, has a source name, and agrees with the existing prepared source
  id. The existing record helper remains as compatibility surface and now
  delegates through the authority helper.
- `src/backend/mir/x86/module/module.cpp`: the two x86 direct extern scalar
  call-argument emission paths now pass an agreed Route 6 source name as the
  explicit source authority. Missing or non-agreeing Route 6 authority passes
  `std::nullopt`, preserving the existing prepared argument-name fallback.
- `src/backend/mir/x86/debug/debug.cpp`: route-debug now reports
  `gate=missing_source_name` for records with an agreed source id shape but no
  named source value, keeping that incomplete case fail-closed and visible.

Changed test surface:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`: asserts the
  new authority helper fails closed when a Route 6 record has the agreed source
  id but no source name, while the prepared call-argument selector remains
  available.
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`:
  asserts the positive scalar source becomes named Route 6 authority and that
  the missing-source-name case falls back to prepared emission with unchanged
  `expected_minimal_direct_extern_call_lane_asm()`.
- `tests/backend/bir/backend_x86_route_debug_test.cpp`: adds the
  `status=available gate=missing_source_name` row and confirms it does not emit
  the accepted `route6 scalar arg ... kind=ArgumentValue` source row.

Wrapper assembly expectations, prepared fallback behavior, public
`ConsumedPlans` compatibility, and non-agreeing Route 6 fail-closed behavior
were preserved.

## Suggested Next

Next coherent packet: Step 4 should prove wrapper output and prepared
compatibility stability around the selected scalar `i32` direct-call argument
path, with particular attention to `expected_minimal_direct_extern_call_lane_asm()`
and retained public `ConsumedPlans` compatibility rows.

## Watchouts

- Do not broaden this into x86 call-wrapper migration or riscv convergence.
- Do not claim prepared aggregate or draft retirement.
- Do not weaken expected strings, helper-oracle statuses, supported-path
  contracts, fallback behavior, or wrapper assembly as proof.
- Route 6 source authority is now limited to named, argument-value Route 6
  records whose source id agrees with the prepared call argument source id.
- Missing Route 6 facts, missing source ids, missing source names, duplicate
  records, prepared-source mismatches, and source-name mismatches must continue
  to preserve prepared fallback and avoid false positive Route 6 authority.
- `status=available gate=missing_source_value` is intentionally a consumer-side
  agreement-gate result: the raw Route 6 lookup found a relationship, but the
  selected relationship lacks the source id required for x86/prepared
  agreement.
- `status=available gate=missing_source_name`,
  `status=available gate=prepared_source_mismatch`, and
  `status=available gate=source_value_mismatch` distinguish post-lookup
  authority-gate failures from raw Route 6 lookup failures.
- Keep `ConsumedPlans` public compatibility visible; do not hide or demote
  `PreparedFunctionLookups`, prepared call plans, or fallback labels in this
  idea.
- `clang-format` was not available in this environment
  (`/bin/bash: clang-format: command not found`); formatting was kept manual.

## Proof

Passed:

`cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test backend_x86_handoff_boundary_test && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_prepared_lookup_helper|backend_x86_handoff_boundary)$' | tee test_after.log`

`test_after.log` is the proof log path. The selected subset passed 3/3:
`backend_x86_route_debug`, `backend_prepared_lookup_helper`, and
`backend_x86_handoff_boundary`.
