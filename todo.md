Status: Active
Source Idea Path: ideas/open/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove wrapper output and prepared compatibility stability

# Current Packet

## Just Finished

Step 4 - Prove wrapper output and prepared compatibility stability completed.

The delegated x86 wrapper/prepared compatibility proof stayed green after the
Route 6 scalar source authority change:

- wrapper output stability around
  `expected_minimal_direct_extern_call_lane_asm()` remains covered by
  `backend_x86_handoff_boundary`
- retained public `ConsumedPlans` compatibility remains covered by
  `backend_prepared_lookup_helper`
- prepared fallback stability and route-debug fail-closed visibility remain
  covered by `backend_x86_route_debug`, `backend_prepared_lookup_helper`, and
  `backend_x86_handoff_boundary`
- no code, test, `plan.md`, or idea-file edits were made in this proof-only
  packet

## Suggested Next

Next coherent packet: Step 5 should run the close-scope regression guard for
the completed selected scalar `i32` x86 path, confirming no supported-path
downgrade, helper-oracle weakening, wrapper output rewrite, or prepared
aggregate demotion claim entered the diff.

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
- Step 4 was proof-only; it did not inspect or modify implementation/test
  content beyond the delegated build and test subset.
- `clang-format` was not available in this environment
  (`/bin/bash: clang-format: command not found`); formatting was kept manual.

## Proof

Passed:

`cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test backend_x86_handoff_boundary_test && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_prepared_lookup_helper|backend_x86_handoff_boundary)$' | tee test_after.log`

`test_after.log` is the proof log path. The selected subset passed 3/3:
`backend_x86_route_debug`, `backend_prepared_lookup_helper`, and
`backend_x86_handoff_boundary`. This proof is sufficient for Step 4 because it
re-ran the delegated wrapper output, prepared lookup compatibility, and x86
route-debug/fallback stability subset without changing code or tests.

Baseline review accepted this candidate because the full-suite candidate stayed
3428/3428 with no failures.
