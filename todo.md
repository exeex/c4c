Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Normalize Projection Naming

Code Review Reminder Handled: review/address-projection-step3-review.md found Step 3 helper reuse aligned and recommended continuing Step 3.
Test Baseline Reminder Handled: accepted full-suite test_baseline.log for commit c97d5e69 after 3071 passed, 0 failed.

# Current Packet

## Just Finished

Step 3 is complete for the active runbook surface.

Completed helper-reuse coverage:

- The constant-index array/struct child traversal before a local dynamic
  pointer-array index in `resolve_local_aggregate_dynamic_pointer_array_access`
  now consumes `resolve_aggregate_child_index_projection` while preserving the
  old zero-sized array-element rejection.
- The constant-index tail walk after the dynamic array index also consumes
  `resolve_aggregate_child_index_projection`.
- The first-index policy, dynamic-index lowering, pointer-leaf return policy,
  and local slot publication policy remain local to the caller path.
- The dynamic tail walk keeps its existing layout validity guards before using
  the helper for child offset and next-type projection.
- The dynamic aggregate array projection surface named by the Step 3 review is
  already covered by the shared extent/projection helpers: global and local
  dynamic aggregate array access both use
  `find_repeated_aggregate_extent_at_offset`, and dynamic local aggregate GEP
  projection reuses `resolve_local_aggregate_gep_slot`, whose constant child
  traversal now consumes `resolve_aggregate_child_index_projection`.
- No remaining Step 3 packet is needed before naming normalization. The
  remaining dynamic aggregate array behavior is extent-based and should stay at
  the caller sites unless a later runbook explicitly changes that policy.

## Suggested Next

Begin Step 4 by normalizing projection naming only within the active
consolidation surface touched by Steps 2 and 3. Focus on shared terms for byte
offset, child type/index, element stride, repeated extent, and scalar leaf
facts while avoiding cosmetic churn outside the updated helper/call-site code.

## Watchouts

- Preserve BIR output, diagnostics, and testcase expectations.
- Keep `BirFunctionLowerer` as the stateful lowering owner.
- Prefer existing `memory_helpers.hpp`; do not add a new header unless the
  active plan is explicitly revised.
- Do not turn the inventory into testcase-shaped matching.
- Do not move publication policy into pure helpers. Helpers should report facts;
  callers should still decide which state map to update.
- Be careful with first-index policy: local aggregate GEP, absolute global GEP,
  relative global GEP, dynamic aggregate GEP, and runtime pointer provenance use
  similar layout math but different acceptance rules.
- Treat dynamic aggregate array access as extent-based unless future execution
  finds a genuine duplicated projection walk that is not already routed through
  `find_repeated_aggregate_extent_at_offset` or `resolve_local_aggregate_gep_slot`.

## Proof

Ran the delegated proof command and wrote combined stdout/stderr to
`test_after.log`:

`cmake --build --preset default --target c4c_codegen c4c_backend c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. CTest reported 100% tests passed, 0 failed out of 97 run;
12 matching backend tests were disabled and not run.
